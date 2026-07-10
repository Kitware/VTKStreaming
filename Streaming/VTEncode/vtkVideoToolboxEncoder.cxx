/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoToolboxEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This module is only added to the build on Apple platforms (see the top-level
// CMakeLists.txt `if(APPLE)` guard), so VideoToolbox is always available here.

#include "vtkVideoToolboxEncoder.h"

#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>
#include <VideoToolbox/VideoToolbox.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace
{
//------------------------------------------------------------------------------
// Small RAII helper so CoreFoundation objects are released on scope exit.
template <typename T>
struct CFHandle
{
  T Ref = nullptr;
  CFHandle() = default;
  explicit CFHandle(T ref)
    : Ref(ref)
  {
  }
  ~CFHandle()
  {
    if (this->Ref != nullptr)
    {
      CFRelease(this->Ref);
    }
  }
  CFHandle(const CFHandle&) = delete;
  CFHandle& operator=(const CFHandle&) = delete;
  operator T() const { return this->Ref; }
};

//------------------------------------------------------------------------------
// Set a numeric session property from an int/double/bool.
void SetSessionProperty(VTCompressionSessionRef session, CFStringRef key, int32_t value)
{
  CFHandle<CFNumberRef> number(CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value));
  VTSessionSetProperty(session, key, number);
}
void SetSessionProperty(VTCompressionSessionRef session, CFStringRef key, double value)
{
  CFHandle<CFNumberRef> number(CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &value));
  VTSessionSetProperty(session, key, number);
}
void SetSessionProperty(VTCompressionSessionRef session, CFStringRef key, bool value)
{
  VTSessionSetProperty(session, key, value ? kCFBooleanTrue : kCFBooleanFalse);
}

// Same as above but reports the status, for properties an encoder may reject
// (e.g. ConstantBitRate, which not every hardware encoder supports).
OSStatus SetSessionPropertyChecked(VTCompressionSessionRef session, CFStringRef key, int32_t value)
{
  CFHandle<CFNumberRef> number(CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value));
  return VTSessionSetProperty(session, key, number);
}

// Cap the instantaneous data rate to `bitsPerSecond` averaged over one second.
// Combined with an average bitrate this bounds VBR; used alone it approximates
// CBR on encoders that lack a true constant-bitrate mode.
void SetDataRateLimit(VTCompressionSessionRef session, unsigned int bitsPerSecond)
{
  if (bitsPerSecond == 0)
  {
    return;
  }
  const int64_t bytesPerSecond = bitsPerSecond / 8;
  const double seconds = 1.0;
  CFHandle<CFNumberRef> bytes(
    CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &bytesPerSecond));
  CFHandle<CFNumberRef> secs(CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &seconds));
  const void* limitValues[] = { bytes.Ref, secs.Ref };
  CFHandle<CFArrayRef> limits(
    CFArrayCreate(kCFAllocatorDefault, limitValues, 2, &kCFTypeArrayCallBacks));
  VTSessionSetProperty(session, kVTCompressionPropertyKey_DataRateLimits, limits);
}

//------------------------------------------------------------------------------
// The VideoToolbox pixel format matching a VTKStreaming pixel format, or 0 when
// the format is unsupported.
OSType ToCVPixelFormat(VTKPixelFormatType format)
{
  switch (format)
  {
    case VTKPixelFormatType::VTKPF_NV12:
      return kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
    case VTKPixelFormatType::VTKPF_IYUV:
      return kCVPixelFormatType_420YpCbCr8Planar;
    case VTKPixelFormatType::VTKPF_RGBA32:
      return kCVPixelFormatType_32BGRA;
    case VTKPixelFormatType::VTKPF_RGB24:
    default:
      return 0;
  }
}
} // anonymous namespace

//------------------------------------------------------------------------------
struct vtkVideoToolboxEncoderInternals
{
  VTCompressionSessionRef Session = nullptr;
  CMVideoCodecType CodecType = kCMVideoCodecType_H264;
  // Filled synchronously by the output callback, drained by EncodeInternal.
  std::vector<vtkSmartPointer<vtkCompressedVideoPacket>> PendingPackets;
  VTKVideoProcessingStatusType LastStatus = VTKVideoProcessingStatusType::VTKVPStatus_Success;
  int64_t FrameCounter = 0;
  // RFC 6381 codec string, parsed from the SPS on key frames and reused for the
  // delta frames that follow (which carry no parameter sets).
  std::string CodecName;
  bool Initialized = false;
  // timing for encode and copy ops.
  std::chrono::high_resolution_clock::duration dtEncode{}, dtCopy{};
};

vtkStandardNewMacro(vtkVideoToolboxEncoder);

//------------------------------------------------------------------------------
vtkVideoToolboxEncoder::vtkVideoToolboxEncoder()
  : Internals(new vtkVideoToolboxEncoderInternals())
{
  // Default to a codec this backend supports (matches Internals->CodecType), so an
  // instance created without an explicit SetCodec is valid. The base class default is VP9,
  // which VideoToolbox does not support.
  this->Codec = VTKVideoCodecType::VTKVC_H264;
}

//------------------------------------------------------------------------------
vtkVideoToolboxEncoder::~vtkVideoToolboxEncoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkVideoToolboxEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkIdType vtkVideoToolboxEncoder::GetLastEncodeTimeNS() const noexcept
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(this->Internals->dtEncode).count();
}

//------------------------------------------------------------------------------
vtkIdType vtkVideoToolboxEncoder::GetLastScaleTimeNS() const noexcept
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(this->Internals->dtCopy).count();
}

//------------------------------------------------------------------------------
bool vtkVideoToolboxEncoder::SupportsCodec(VTKVideoCodecType codec) const noexcept
{
  return codec == VTKVideoCodecType::VTKVC_H264 || codec == VTKVideoCodecType::VTKVC_H265;
}

namespace
{
// The 4-byte Annex B NAL unit start code.
const unsigned char kAnnexBStartCode[4] = { 0x00, 0x00, 0x00, 0x01 };

//------------------------------------------------------------------------------
// The byte length of the AVCC/HVCC NAL length prefix VideoToolbox emits (usually
// 4). Reported by the parameter-set accessors on the format description.
int GetNalHeaderLength(CMFormatDescriptionRef format)
{
  if (format == nullptr)
  {
    return 4;
  }
  const CMVideoCodecType codec = CMFormatDescriptionGetMediaSubType(format);
  int nalHeaderLength = 4;
  size_t count = 0;
  if (codec == kCMVideoCodecType_HEVC)
  {
    CMVideoFormatDescriptionGetHEVCParameterSetAtIndex(
      format, 0, nullptr, nullptr, &count, &nalHeaderLength);
  }
  else
  {
    CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
      format, 0, nullptr, nullptr, &count, &nalHeaderLength);
  }
  return nalHeaderLength > 0 ? nalHeaderLength : 4;
}

//------------------------------------------------------------------------------
// Prepend each parameter set NAL (SPS/PPS, plus VPS for HEVC) to `out` as an
// Annex B unit (start code + NAL) so a key frame is self-describing.
void AppendParameterSetsAnnexB(CMSampleBufferRef sampleBuffer, std::vector<unsigned char>& out)
{
  CMFormatDescriptionRef format = CMSampleBufferGetFormatDescription(sampleBuffer);
  if (format == nullptr)
  {
    return;
  }
  const CMVideoCodecType codec = CMFormatDescriptionGetMediaSubType(format);
  size_t count = 0;
  // Query the parameter set count (H.264 and HEVC have dedicated accessors).
  if (codec == kCMVideoCodecType_HEVC)
  {
    if (CMVideoFormatDescriptionGetHEVCParameterSetAtIndex(
          format, 0, nullptr, nullptr, &count, nullptr) != noErr)
    {
      return;
    }
  }
  else
  {
    if (CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
          format, 0, nullptr, nullptr, &count, nullptr) != noErr)
    {
      return;
    }
  }
  for (size_t i = 0; i < count; ++i)
  {
    const uint8_t* ps = nullptr;
    size_t psSize = 0;
    OSStatus status = (codec == kCMVideoCodecType_HEVC)
      ? CMVideoFormatDescriptionGetHEVCParameterSetAtIndex(
          format, i, &ps, &psSize, nullptr, nullptr)
      : CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
          format, i, &ps, &psSize, nullptr, nullptr);
    if (status != noErr || ps == nullptr)
    {
      continue;
    }
    out.insert(out.end(), kAnnexBStartCode, kAnnexBStartCode + 4);
    out.insert(out.end(), ps, ps + psSize);
  }
}

//------------------------------------------------------------------------------
// Convert a run of AVCC/HVCC length-prefixed NAL units into Annex B (each NAL
// prefixed by a start code) and append to `out`. This matches the NVENC backend
// and makes the raw elementary stream directly playable.
void AppendAnnexB(
  const unsigned char* data, size_t length, int nalHeaderLength, std::vector<unsigned char>& out)
{
  size_t offset = 0;
  while (offset + static_cast<size_t>(nalHeaderLength) <= length)
  {
    uint32_t nalLength = 0;
    for (int i = 0; i < nalHeaderLength; ++i)
    {
      nalLength = (nalLength << 8) | data[offset + i];
    }
    offset += nalHeaderLength;
    if (nalLength == 0 || offset + nalLength > length)
    {
      break;
    }
    out.insert(out.end(), kAnnexBStartCode, kAnnexBStartCode + 4);
    out.insert(out.end(), data + offset, data + offset + nalLength);
    offset += nalLength;
  }
}

//------------------------------------------------------------------------------
// Build a WebCodecs/RFC-6381 codec string from the sequence parameter set (SPS)
// carried by an Annex-B key frame bitstream. Returns an empty string when no SPS
// is present (e.g. a delta frame) or the codec is unsupported. Mirrors the
// NVENC backend's BuildCodecString so both encoders report identical strings.
std::string BuildCodecString(CMVideoCodecType codec, const unsigned char* data, std::size_t size)
{
  const bool isH264 = (codec == kCMVideoCodecType_H264);
  const bool isHEVC = (codec == kCMVideoCodecType_HEVC);
  if (data == nullptr || (!isH264 && !isHEVC))
  {
    return {};
  }
  // Walk the Annex-B NAL units looking for the SPS. A leading zero of a 4-byte
  // start code is skipped naturally by advancing one byte at a time.
  for (std::size_t i = 0; i + 3 < size; ++i)
  {
    if (data[i] != 0 || data[i + 1] != 0 || data[i + 2] != 1)
    {
      continue;
    }
    const std::size_t nal = i + 3;
    if (isH264)
    {
      const int nalType = data[nal] & 0x1F;
      if (nalType == 7 && nal + 3 < size) // Sequence parameter set.
      {
        // avc1.PPCCLL: profile_idc, constraint_set flags byte, level_idc.
        char buf[16];
        std::snprintf(
          buf, sizeof(buf), "avc1.%02X%02X%02X", data[nal + 1], data[nal + 2], data[nal + 3]);
        return buf;
      }
    }
    else // HEVC
    {
      const int nalType = (data[nal] >> 1) & 0x3F;
      if (nalType == 33 && nal + 14 < size) // Sequence parameter set.
      {
        // Skip the 2-byte NAL header and the byte holding
        // sps_video_parameter_set_id/sps_max_sub_layers_minus1/
        // sps_temporal_id_nesting_flag to reach profile_tier_level().
        const unsigned char* ptl = &data[nal + 3];
        const int profileSpace = (ptl[0] >> 6) & 0x03;
        const int tierFlag = (ptl[0] >> 5) & 0x01;
        const int profileIdc = ptl[0] & 0x1F;
        const std::uint32_t compat = (std::uint32_t(ptl[1]) << 24) | (std::uint32_t(ptl[2]) << 16) |
          (std::uint32_t(ptl[3]) << 8) | std::uint32_t(ptl[4]);
        const int levelIdc = ptl[11];
        std::ostringstream oss;
        oss << "hvc1.";
        if (profileSpace > 0)
        {
          oss << static_cast<char>('A' + profileSpace - 1);
        }
        oss << profileIdc << '.' << std::uppercase << std::hex << compat << std::dec << '.'
            << (tierFlag ? 'H' : 'L') << levelIdc;
        // Six constraint-indicator-flag bytes, trailing zero bytes trimmed.
        int last = 5;
        while (last >= 0 && ptl[5 + last] == 0)
        {
          --last;
        }
        for (int c = 0; c <= last; ++c)
        {
          char cbuf[8];
          std::snprintf(cbuf, sizeof(cbuf), ".%02X", ptl[5 + c]);
          oss << cbuf;
        }
        return oss.str();
      }
    }
  }
  return {};
}

//------------------------------------------------------------------------------
// Whether a sample buffer holds a sync sample (key frame). Absence of the
// attachment array, or NotSync being false/absent, means it is a key frame.
bool IsKeyFrame(CMSampleBufferRef sampleBuffer)
{
  CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, false);
  if (attachments == nullptr || CFArrayGetCount(attachments) == 0)
  {
    return true;
  }
  CFDictionaryRef dict = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(attachments, 0));
  CFBooleanRef notSync = nullptr;
  if (CFDictionaryGetValueIfPresent(
        dict, kCMSampleAttachmentKey_NotSync, reinterpret_cast<const void**>(&notSync)))
  {
    return !CFBooleanGetValue(notSync);
  }
  return true;
}

//------------------------------------------------------------------------------
// VideoToolbox output callback. Runs synchronously with respect to
// EncodeInternal thanks to VTCompressionSessionCompleteFrames.
void CompressionOutputCallback(void* outputCallbackRefCon, void* /*sourceFrameRefCon*/,
  OSStatus status, VTEncodeInfoFlags infoFlags, CMSampleBufferRef sampleBuffer)
{
  auto* internals = static_cast<vtkVideoToolboxEncoderInternals*>(outputCallbackRefCon);
  if (status != noErr)
  {
    vtkLogF(ERROR, "VideoToolbox encode failed with OSStatus %d", static_cast<int>(status));
    internals->LastStatus = VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    return;
  }
  if ((infoFlags & kVTEncodeInfo_FrameDropped) != 0 || sampleBuffer == nullptr ||
    !CMSampleBufferDataIsReady(sampleBuffer))
  {
    return;
  }

  CMBlockBufferRef block = CMSampleBufferGetDataBuffer(sampleBuffer);
  if (block == nullptr)
  {
    return;
  }
  size_t totalLength = 0;
  char* dataPointer = nullptr;
  if (CMBlockBufferGetDataPointer(block, 0, nullptr, &totalLength, &dataPointer) != noErr ||
    dataPointer == nullptr)
  {
    return;
  }

  const bool keyFrame = IsKeyFrame(sampleBuffer);
  CMFormatDescriptionRef format = CMSampleBufferGetFormatDescription(sampleBuffer);
  const int nalHeaderLength = GetNalHeaderLength(format);
  std::vector<unsigned char> payload;
  payload.reserve(totalLength + 256);
  if (keyFrame)
  {
    // Inline the parameter sets ahead of the key frame (Annex B).
    AppendParameterSetsAnnexB(sampleBuffer, payload);
  }
  // Rewrite the AVCC/HVCC length prefixes as Annex B start codes so the stream
  // matches the NVENC backend and plays directly.
  AppendAnnexB(
    reinterpret_cast<const unsigned char*>(dataPointer), totalLength, nalHeaderLength, payload);

  CMTime pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
  CMVideoDimensions dims = { 0, 0 };
  if (format != nullptr)
  {
    dims = CMVideoFormatDescriptionGetDimensions(format);
  }

  // Key frames carry the SPS; parse the RFC 6381 codec string from it and reuse
  // it for the delta frames that follow.
  if (keyFrame)
  {
    std::string codecName = BuildCodecString(internals->CodecType, payload.data(), payload.size());
    if (!codecName.empty())
    {
      internals->CodecName = codecName;
    }
  }

  auto chunk = vtk::TakeSmartPointer(vtkCompressedVideoPacket::New());
  chunk->SetIsKeyFrame(keyFrame);
  chunk->SetCodecLongName(internals->CodecName.c_str());
  chunk->SetDisplayWidth(dims.width);
  chunk->SetDisplayHeight(dims.height);
  chunk->SetCodedWidth(dims.width);
  chunk->SetCodedHeight(dims.height);
  chunk->SetPresentationTS(CMTIME_IS_VALID(pts) ? pts.value : internals->FrameCounter);
  chunk->CopyData(payload.data(), static_cast<int>(payload.size()));
  vtkLogF(TRACE, "%lld|%s|%d bytes", chunk->GetPresentationTS(),
    chunk->GetIsKeyFrame() ? "key" : "delta", chunk->GetSize());
  internals->PendingPackets.emplace_back(chunk);
}
} // anonymous namespace

//------------------------------------------------------------------------------
bool vtkVideoToolboxEncoder::InitializeInternal()
{
  auto& internals = (*this->Internals);
  internals.FrameCounter = 0;
  internals.CodecName.clear();
  internals.LastStatus = VTKVideoProcessingStatusType::VTKVPStatus_Success;
  switch (this->Codec)
  {
    case VTKVideoCodecType::VTKVC_H264:
      internals.CodecType = kCMVideoCodecType_H264;
      break;
    case VTKVideoCodecType::VTKVC_H265:
      internals.CodecType = kCMVideoCodecType_HEVC;
      break;
    default:
      vtkLogF(ERROR, "VideoToolbox encoder only supports H.264 and H.265. Got codec %s.",
        vtkVideoCodecTypeUtilities::ToString(this->Codec));
      return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkVideoToolboxEncoder::ShutdownInternal()
{
  auto& internals = (*this->Internals);
  internals.PendingPackets.clear();
  internals.Initialized = false;
}

//------------------------------------------------------------------------------
bool vtkVideoToolboxEncoder::SetupEncoderFrame(int width, int height)
{
  vtkLogF(TRACE, "%s, %dx%d", __func__, width, height);
  auto& internals = (*this->Internals);

  const OSType cvFormat = ToCVPixelFormat(this->InputPixelFormat);
  if (cvFormat == 0)
  {
    vtkLogF(ERROR, "Unsupported input pixel format %s for VideoToolbox.",
      vtkPixelFormatTypeUtilities::ToString(this->InputPixelFormat));
    return false;
  }

  // Ask the compression session to hand us IOSurface-backed pixel buffers of
  // the source format, which is what the hardware encoder path expects.
  const void* keys[] = { kCVPixelBufferPixelFormatTypeKey, kCVPixelBufferWidthKey,
    kCVPixelBufferHeightKey, kCVPixelBufferIOSurfacePropertiesKey };
  CFHandle<CFNumberRef> formatValue(
    CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &cvFormat));
  CFHandle<CFNumberRef> widthValue(CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &width));
  CFHandle<CFNumberRef> heightValue(CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &height));
  CFHandle<CFDictionaryRef> ioSurfaceProps(CFDictionaryCreate(kCFAllocatorDefault, nullptr, nullptr,
    0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
  const void* values[] = { formatValue.Ref, widthValue.Ref, heightValue.Ref, ioSurfaceProps.Ref };
  CFHandle<CFDictionaryRef> sourceAttrs(CFDictionaryCreate(kCFAllocatorDefault, keys, values, 4,
    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

  // Request a hardware-accelerated encoder.
  const void* encKeys[] = { kVTVideoEncoderSpecification_EnableHardwareAcceleratedVideoEncoder };
  const void* encValues[] = { kCFBooleanTrue };
  CFHandle<CFDictionaryRef> encoderSpec(CFDictionaryCreate(kCFAllocatorDefault, encKeys, encValues,
    1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

  OSStatus status =
    VTCompressionSessionCreate(kCFAllocatorDefault, width, height, internals.CodecType, encoderSpec,
      sourceAttrs, nullptr, &CompressionOutputCallback, &internals, &internals.Session);
  if (status != noErr || internals.Session == nullptr)
  {
    vtkLogF(ERROR, "VTCompressionSessionCreate failed with OSStatus %d", static_cast<int>(status));
    return false;
  }

  VTCompressionSessionRef session = internals.Session;
  SetSessionProperty(session, kVTCompressionPropertyKey_RealTime, this->LowDelayMode);
  SetSessionProperty(
    session, kVTCompressionPropertyKey_AllowFrameReordering, this->MaximumBFrames > 0);
  SetSessionProperty(
    session, kVTCompressionPropertyKey_MaxKeyFrameInterval, this->GroupOfPicturesSize);
  if (this->TimeBaseEnd > 0)
  {
    SetSessionProperty(session, kVTCompressionPropertyKey_ExpectedFrameRate,
      static_cast<int32_t>(this->TimeBaseEnd));
  }
  // Rate control. VideoToolbox is bitrate-driven and, unlike NVENC/libvpx, has
  // no public constant-QP mode, so the QP-based modes are approximated.
  switch (this->BitRateControlMode)
  {
    case vtkVideoEncoder::BRCType::CBR:
    {
      // True CBR needs kVTCompressionPropertyKey_ConstantBitRate (macOS 13+),
      // and not every hardware encoder honors it; fall back to an average
      // bitrate hard-capped by a one-second data-rate limit.
      bool cbrSet = false;
      if (__builtin_available(macOS 13.0, *))
      {
        cbrSet = this->BitRate > 0 &&
          SetSessionPropertyChecked(session, kVTCompressionPropertyKey_ConstantBitRate,
            static_cast<int32_t>(this->BitRate)) == noErr;
      }
      if (!cbrSet)
      {
        if (this->BitRate > 0)
        {
          SetSessionProperty(
            session, kVTCompressionPropertyKey_AverageBitRate, static_cast<int32_t>(this->BitRate));
        }
        SetDataRateLimit(session, this->BitRate);
      }
      break;
    }
    case vtkVideoEncoder::BRCType::CQP:
    case vtkVideoEncoder::BRCType::QP:
    {
      // No fixed-QP mode exists; map the base-class quantizer to a quality hint
      // (higher QP -> lower quality) and, where available, bound the frame QP.
      vtkLog(TRACE,
        "VideoToolbox has no constant-QP mode; approximating with a quality hint "
        "derived from QuantizationParameter.");
      const double quality =
        1.0 - (vtkMath::ClampValue(this->QuantizationParameter, 1u, 63u) / 63.0);
      SetSessionProperty(session, kVTCompressionPropertyKey_Quality, quality);
      if (__builtin_available(macOS 13.0, *))
      {
        SetSessionPropertyChecked(session, kVTCompressionPropertyKey_MinAllowedFrameQP,
          static_cast<int32_t>(this->MinQuantizationParameter));
        SetSessionPropertyChecked(session, kVTCompressionPropertyKey_MaxAllowedFrameQP,
          static_cast<int32_t>(this->MaxQuantizationParameter));
      }
      break;
    }
    case vtkVideoEncoder::BRCType::VBR:
    default:
      if (this->BitRate > 0)
      {
        SetSessionProperty(
          session, kVTCompressionPropertyKey_AverageBitRate, static_cast<int32_t>(this->BitRate));
      }
      // A higher ceiling than the average allows the rate to fluctuate (VBR).
      if (this->MaxBitRate > this->BitRate)
      {
        SetDataRateLimit(session, this->MaxBitRate);
      }
      break;
  }

  VTCompressionSessionPrepareToEncodeFrames(session);
  internals.Initialized = true;
  return true;
}

//------------------------------------------------------------------------------
void vtkVideoToolboxEncoder::TearDownEncoderFrame()
{
  auto& internals = (*this->Internals);
  if (internals.Session != nullptr)
  {
    VTCompressionSessionCompleteFrames(internals.Session, kCMTimeInvalid);
    VTCompressionSessionInvalidate(internals.Session);
    CFRelease(internals.Session);
    internals.Session = nullptr;
  }
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVideoToolboxEncoder::EncodeInternal(
  vtkSmartPointer<vtkRawVideoFrame> frame)
{
  auto& internals = (*this->Internals);
  if (frame == nullptr || internals.Session == nullptr)
  {
    return {};
  }

  // Obtain an IOSurface-backed pixel buffer from the session pool.
  CVPixelBufferPoolRef pool = VTCompressionSessionGetPixelBufferPool(internals.Session);
  if (pool == nullptr)
  {
    vtkLog(ERROR, "VideoToolbox pixel buffer pool is unavailable.");
    return { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} };
  }
  CVPixelBufferRef pixelBuffer = nullptr;
  if (CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pool, &pixelBuffer) !=
      kCVReturnSuccess ||
    pixelBuffer == nullptr)
  {
    vtkLog(ERROR, "Failed to allocate a VideoToolbox pixel buffer.");
    return { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} };
  }

  // Copy the (CPU-resident) frame pixels into the pixel buffer plane by plane.
  auto tc1 = std::chrono::high_resolution_clock::now();
  unsigned char* src = nullptr;
  const unsigned int srcSize = frame->GetData(src);
  const int* strides = frame->GetStrides();
  const VTKPixelFormatType format = frame->GetPixelFormat();
  const int height = frame->GetHeight();

  CVPixelBufferLockBaseAddress(pixelBuffer, 0);
  if (format == VTKPixelFormatType::VTKPF_RGBA32)
  {
    // Single packed plane; swizzle RGBA -> BGRA to match kCVPixelFormatType_32BGRA.
    auto* dst = static_cast<unsigned char*>(CVPixelBufferGetBaseAddress(pixelBuffer));
    const size_t dstStride = CVPixelBufferGetBytesPerRow(pixelBuffer);
    const int srcStride = strides[0];
    const int copyBytes = std::min<int>(srcStride, static_cast<int>(dstStride));
    for (int row = 0; row < height; ++row)
    {
      const unsigned char* s = src + (static_cast<size_t>(row) * srcStride);
      unsigned char* d = dst + (static_cast<size_t>(row) * dstStride);
      for (int x = 0; x + 3 < copyBytes; x += 4)
      {
        d[x + 0] = s[x + 2]; // B
        d[x + 1] = s[x + 1]; // G
        d[x + 2] = s[x + 0]; // R
        d[x + 3] = s[x + 3]; // A
      }
    }
  }
  else
  {
    // Planar YUV (NV12: 2 planes, IYUV: 3 planes).
    const size_t planeCount = CVPixelBufferGetPlaneCount(pixelBuffer);
    for (size_t plane = 0; plane < planeCount; ++plane)
    {
      auto* dst =
        static_cast<unsigned char*>(CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, plane));
      const size_t dstStride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, plane);
      const size_t planeHeight = CVPixelBufferGetHeightOfPlane(pixelBuffer, plane);
      const int srcStride = strides[plane];
      const unsigned char* s = src + frame->GetPlanePointerIdx(static_cast<int>(plane));
      const int copyBytes = std::min<int>(srcStride, static_cast<int>(dstStride));
      for (size_t row = 0; row < planeHeight; ++row)
      {
        const unsigned char* srcRow = s + (row * srcStride);
        std::copy(srcRow, srcRow + copyBytes, dst + (row * dstStride));
      }
    }
  }
  CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
  delete[] src;
  (void)srcSize;
  auto tc2 = std::chrono::high_resolution_clock::now();
  internals.dtCopy = tc2 - tc1;

  // Optionally force a key frame.
  CFHandle<CFDictionaryRef> frameProps;
  if (this->ForceIFrame || this->KeyFramesOnly)
  {
    const void* keys[] = { kVTEncodeFrameOptionKey_ForceKeyFrame };
    const void* values[] = { kCFBooleanTrue };
    frameProps.Ref = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1,
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  }

  const int32_t timescale = this->TimeBaseEnd > 0 ? this->TimeBaseEnd : 30;
  const CMTime pts = CMTimeMake(internals.FrameCounter++, timescale);
  const CMTime duration = CMTimeMake(1, timescale);

  auto te1 = std::chrono::high_resolution_clock::now();
  internals.PendingPackets.clear();
  internals.LastStatus = VTKVideoProcessingStatusType::VTKVPStatus_Success;
  VTEncodeInfoFlags infoFlags = 0;
  OSStatus status = VTCompressionSessionEncodeFrame(
    internals.Session, pixelBuffer, pts, duration, frameProps, nullptr, &infoFlags);
  CVPixelBufferRelease(pixelBuffer);
  if (status != noErr)
  {
    vtkLogF(
      ERROR, "VTCompressionSessionEncodeFrame failed with OSStatus %d", static_cast<int>(status));
    return { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} };
  }
  // Force synchronous delivery so packets are available to return now.
  VTCompressionSessionCompleteFrames(internals.Session, kCMTimeInvalid);
  auto te2 = std::chrono::high_resolution_clock::now();
  internals.dtEncode = te2 - te1;

  VTKVideoEncoderResultType result;
  result.first = internals.LastStatus;
  result.second = internals.PendingPackets;
  internals.PendingPackets.clear();
  return result;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVideoToolboxEncoder::SendEOS()
{
  auto& internals = (*this->Internals);
  if (internals.Session == nullptr)
  {
    return {};
  }
  internals.PendingPackets.clear();
  internals.LastStatus = VTKVideoProcessingStatusType::VTKVPStatus_Success;
  VTCompressionSessionCompleteFrames(internals.Session, kCMTimeInvalid);
  VTKVideoEncoderResultType result;
  result.first = internals.LastStatus;
  result.second = internals.PendingPackets;
  internals.PendingPackets.clear();
  return result;
}

//------------------------------------------------------------------------------
bool vtkVideoToolboxEncoder::CheckAvailability() noexcept
{
  // Try to create a small hardware-accelerated H.264 session; success means a
  // usable VideoToolbox encoder is present.
  const void* encKeys[] = { kVTVideoEncoderSpecification_EnableHardwareAcceleratedVideoEncoder };
  const void* encValues[] = { kCFBooleanTrue };
  CFHandle<CFDictionaryRef> encoderSpec(CFDictionaryCreate(kCFAllocatorDefault, encKeys, encValues,
    1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
  VTCompressionSessionRef session = nullptr;
  OSStatus status = VTCompressionSessionCreate(kCFAllocatorDefault, 640, 480,
    kCMVideoCodecType_H264, encoderSpec, nullptr, nullptr, nullptr, nullptr, &session);
  if (status == noErr && session != nullptr)
  {
    VTCompressionSessionInvalidate(session);
    CFRelease(session);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
// TEMP (VTK 9.6): self-register this backend with vtkEncoderFactory so it can be
// selected by preferences. Delete this block for VTK 9.7 and instead return these
// attributes from vtkVideoToolboxEncoder::CreateOverrideAttributes().
#include "vtkEncoderFactory.h"
namespace
{
vtkVideoEncoder* CreateVideoToolboxEncoder()
{
  return vtkVideoToolboxEncoder::New();
}

struct vtkVideoToolboxEncoderRegistrar
{
  vtkVideoToolboxEncoderRegistrar()
  {
    vtkEncoderFactory::BackendDescriptor d;
    d.SubclassName = "vtkVideoToolboxEncoder";
    d.Create = &CreateVideoToolboxEncoder;
    d.Available = &vtkVideoToolboxEncoder::CheckAvailability;
    d.Hardware = true;
    d.Codecs = { VTKVideoCodecType::VTKVC_H264, VTKVideoCodecType::VTKVC_H265 };
    d.Attributes = { { "Platform", "macOS" }, { "Hardware", "true" } };
    vtkEncoderFactory::RegisterBackend(d);
  }
};
// Runs when the vtkStreamingVTEncode library is loaded (e.g. on `import vtk_streaming`).
const vtkVideoToolboxEncoderRegistrar sVideoToolboxEncoderRegistrar;
} // anonymous namespace
