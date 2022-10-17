/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFmpegHardwareEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFmpegHardwareEncoder.h"
#include "vtkFFmpegEncoderInternals.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtksys/SystemInformation.hxx"

extern "C"
{
#include <libavutil/hwcontext.h>
#include <libavutil/log.h>
#include <libavutil/pixfmt.h>
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFFmpegHardwareEncoder);

//------------------------------------------------------------------------------
vtkFFmpegHardwareEncoder::vtkFFmpegHardwareEncoder()
  : Internals(new vtkFFmpegEncoderInternals())
{
  if (vtkLogger::GetCurrentVerbosityCutoff() == vtkLogger::VERBOSITY_ERROR)
  {
    av_log_set_level(AV_LOG_ERROR);
  }
  else if (vtkLogger::GetCurrentVerbosityCutoff() == vtkLogger::VERBOSITY_INFO)
  {
    av_log_set_level(AV_LOG_INFO);
  }
  else if (vtkLogger::GetCurrentVerbosityCutoff() == vtkLogger::VERBOSITY_WARNING)
  {
    av_log_set_level(AV_LOG_WARNING);
  }
  else if (vtkLogger::GetCurrentVerbosityCutoff() == vtkLogger::VERBOSITY_TRACE)
  {
    av_log_set_level(AV_LOG_TRACE);
  }
}

//------------------------------------------------------------------------------
vtkFFmpegHardwareEncoder::~vtkFFmpegHardwareEncoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkFFmpegHardwareEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

//------------------------------------------------------------------------------
bool vtkFFmpegHardwareEncoder::QueryPlatformSupport()
{
  vtkLogScopeFunction(TRACE);
  bool probeOS = false;
  this->HWEncoderType = HardwareEncoderTypeEnum::None;

  switch (this->PreferredGPU)
  {
    case DesktopGPUVendor::AMD:
      vtkLog(TRACE, << "AMD GPU preferred.");
      this->HWEncoderType = HardwareEncoderTypeEnum::AMF;
      break;
    case DesktopGPUVendor::Intel:
      vtkLog(TRACE, << "Intel GPU preferred.");
      // TODO: find proper tweaks to get qsv working on linux and windows. (something to do with
      // device name..)
      this->HWEncoderType = HardwareEncoderTypeEnum::QSV;
      probeOS = true;
      break;
    case DesktopGPUVendor::NVIDIA:
      vtkLog(TRACE, << "NVIDIA GPU preferred.");
      this->HWEncoderType = HardwareEncoderTypeEnum::NVENC;
      break;
    default:
      probeOS = true;
      vtkLog(TRACE, << "No GPU preference.");
      break;
  }

  if (!probeOS)
  {
    vtkLog(TRACE, << "Found a hardware encoder due to GPU preference.");
    return true;
  }
  vtkLog(TRACE, << "Probing OS to determine a hardware encoder.");

  vtksys::SystemInformation systemInfo;
  if (systemInfo.GetOSIsLinux())
  { // vaapi is the best effort on linux.
    this->HWEncoderType = HardwareEncoderTypeEnum::VAAPI;
  }
  else if (systemInfo.GetOSIsApple())
  { // VideoToolbox is the best effort on apple.
    this->HWEncoderType = HardwareEncoderTypeEnum::VideoToolbox;
  }
  else if (systemInfo.GetOSIsWindows())
  { // MediaFoundation is the best effort on windows.
    this->HWEncoderType = HardwareEncoderTypeEnum::MediaFoundation;
  }
  else
  {
    vtkLog(ERROR, << "Failed to infer operating system. Disabling hardware encoder.");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkFFmpegHardwareEncoder::ClearGPUPreference()
{
  vtkLogScopeFunction(TRACE);
  this->PreferredGPU = DesktopGPUVendor::None;
}

//------------------------------------------------------------------------------
void vtkFFmpegHardwareEncoder::PreferAMDEncoders()
{
  vtkLogScopeFunction(TRACE);
  this->PreferredGPU = DesktopGPUVendor::AMD;
}

//------------------------------------------------------------------------------
void vtkFFmpegHardwareEncoder::PreferNVIDIAEncoders()
{
  vtkLogScopeFunction(TRACE);
  this->PreferredGPU = DesktopGPUVendor::NVIDIA;
}

//------------------------------------------------------------------------------
void vtkFFmpegHardwareEncoder::PreferIntelEncoders()
{
  vtkLogScopeFunction(TRACE);
  this->PreferredGPU = DesktopGPUVendor::Intel;
}

//------------------------------------------------------------------------------
bool vtkFFmpegHardwareEncoder::InitializeInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  if (!this->QueryPlatformSupport())
  {
    vtkLog(ERROR, << "Failed to find a hardware encoder suitable for your OS and GPU.");
    return false;
  }

  bool success = true;
  switch (this->InputPixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
      internals.InputPixFmt = AV_PIX_FMT_YUV420P;
      break;
    case VTKPixelFormatType::VTKPF_NV12:
      internals.InputPixFmt = AV_PIX_FMT_NV12;
      break;
    case VTKPixelFormatType::VTKPF_RGBA32:
    case VTKPixelFormatType::VTKPF_RGB24:
      internals.InputPixFmt = AV_PIX_FMT_YUV420P;
      break;
  }
  switch (this->HWEncoderType)
  {
    case HardwareEncoderTypeEnum::VAAPI:
      switch (this->Codec)
      {
        case VTKVideoCodecType::VTKVC_H264:
          internals.CodecName = "h264_vaapi";
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_VAAPI);
          break;
        case VTKVideoCodecType::VTKVC_H265:
          internals.CodecName = "hevc_vaapi";
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_VAAPI);
          break;
        case VTKVideoCodecType::VTKVC_AV1:
          vtkLog(ERROR, << "AV1 is not supported by VAAPI hardware encoder");
          success = false;
          break;
        case VTKVideoCodecType::VTKVC_VP9:
        default:
          internals.CodecName = "vp9_vaapi";
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_VAAPI);
          break;
      }
      break;
    case HardwareEncoderTypeEnum::QSV:
      switch (this->Codec)
      {
        case VTKVideoCodecType::VTKVC_H264:
          internals.CodecName = "h264_qsv";
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_QSV);
          break;
        case VTKVideoCodecType::VTKVC_H265:
          internals.CodecName = "hevc_qsv";
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_QSV);
          break;
        case VTKVideoCodecType::VTKVC_AV1:
          vtkLog(ERROR, << "AV1 is not supported by QSV hardware encoder");
          success = false;
          break;
        case VTKVideoCodecType::VTKVC_VP9:
        default:
          internals.CodecName = "vp9_qsv";
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_QSV);
          break;
      }
      break;
    case HardwareEncoderTypeEnum::AMF:
      switch (this->Codec)
      {
        case VTKVideoCodecType::VTKVC_H264:
          internals.CodecName = "h264_amf";
          success = false; // TODO: Confirm on AMD gpu.
          break;
        case VTKVideoCodecType::VTKVC_H265:
          internals.CodecName = "hevc_amf";
          success = false; // TODO: Confirm on AMD gpu.
          break;
        case VTKVideoCodecType::VTKVC_AV1:
          vtkLog(ERROR, << "AV1 is not supported by AMF hardware encoder");
          success = false;
          break;
        case VTKVideoCodecType::VTKVC_VP9:
        default:
          vtkLog(ERROR, << "VP9 is not supported by AMF hardware encoder");
          success = false;
          break;
      }
      break;
    case HardwareEncoderTypeEnum::NVENC:
      switch (this->Codec)
      {
        case VTKVideoCodecType::VTKVC_H264:
          internals.CodecName = "h264_nvenc";
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_CUDA);
          break;
        case VTKVideoCodecType::VTKVC_H265:
          internals.CodecName = "hevc_nvenc";
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_CUDA);
          break;
        case VTKVideoCodecType::VTKVC_AV1:
          vtkLog(ERROR, << "AV1 is not supported by NVENC hardware encoder");
          success = false;
          break;
        case VTKVideoCodecType::VTKVC_VP9:
        default:
          vtkLog(ERROR, << "VP9 is not supported by NVENC hardware encoder");
          success = false;
          break;
      }
      break;
    case HardwareEncoderTypeEnum::VideoToolbox:
      switch (this->Codec)
      {
        case VTKVideoCodecType::VTKVC_H264:
          internals.CodecName = "h264_videotoolbox";
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
          success = false; // TODO: Confirm on mac.
          break;
        case VTKVideoCodecType::VTKVC_H265:
          internals.CodecName = "hevc_videotoolbox";
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
          success = false; // TODO: Confirm on mac.
          break;
        case VTKVideoCodecType::VTKVC_AV1:
          vtkLog(ERROR, << "AV1 is not supported by VideoToolbox hardware encoder");
          success = false;
          break;
        case VTKVideoCodecType::VTKVC_VP9:
        default:
          vtkLog(ERROR, << "VP9 is not supported by VideoToolbox hardware encoder");
          success = false; // TODO: Confirm on mac.
          break;
      }
      break;
    case HardwareEncoderTypeEnum::MediaFoundation:
      switch (this->Codec)
      {
        case VTKVideoCodecType::VTKVC_H264:
          internals.CodecName = "h264_mediafoundation";
          success = false; // TODO: Confirm on windows.
          break;
        case VTKVideoCodecType::VTKVC_H265:
          internals.CodecName = "hevc_mediafoundation";
          success = false; // TODO: Confirm on windows.
          break;
        case VTKVideoCodecType::VTKVC_AV1:
          vtkLog(ERROR, << "AV1 is not supported by MediaFoundation encoder");
          success = false;
          break;
        case VTKVideoCodecType::VTKVC_VP9:
        default:
          vtkLog(ERROR, << "VP9 is not supported by MediaFoundation encoder");
          success = false; // TODO: Confirm on windows.
          break;
      }
      break;
    default:
      break;
  }

  return success;
}

//------------------------------------------------------------------------------
void vtkFFmpegHardwareEncoder::ShutdownInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.Shutdown();
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkFFmpegHardwareEncoder::EncodeInternal(
  vtkSmartPointer<vtkRawVideoFrame> frame)
{
  vtkLogScopeFunction(TRACE);
  VTKVideoEncoderResultType result;
  auto& internals = *(this->Internals);
  const int64_t pts = (internals.SendCounter++ % this->TimeBaseEnd) + 1;
  internals.SoftwareFrame->pts = pts ? pts : this->TimeBaseEnd;
  internals.HardwareFrame->pts = internals.SoftwareFrame->pts;

  if (!internals.PreprocessInput(frame))
  {
    vtkLog(ERROR, << "Failed to convert rgba32 to encoder input frame pixel format.");
    result.first = VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
    result.second = {};
    return result;
  }

  if (!internals.PrepareForEncoding())
  {
    result.first = VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
    result.second = {};
    return result;
  }

  return internals.Encode(this->ForceIFrame);
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkFFmpegHardwareEncoder::SendEOS()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  VTKVideoEncoderResultType result;
  int ret = avcodec_send_frame(internals.EncodeCtx, nullptr);
  while (ret >= 0)
  {
    ret = avcodec_receive_packet(internals.EncodeCtx, internals.Packet);
    result.second.emplace_back(vtk::TakeSmartPointer(internals.PackageCompressedPacket()));
    switch (ret)
    {
      case AVERROR(EAGAIN):
        result.first = VTKVideoProcessingStatusType::VTKVPStatus_TrySendAgain;
        break;
      case AVERROR(EINVAL):
        result.first = VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
        break;
      case AVERROR_EOF:
        vtkLog(TRACE, "Draining complete");
        result.first = VTKVideoProcessingStatusType::VTKVPStatus_Success;
        ret = -1;
        break;
      default:
        break;
    }
  }
  return result;
}

//------------------------------------------------------------------------------
bool vtkFFmpegHardwareEncoder::SetupEncoderFrame(int width, int height)
{
  vtkLogScopeF(TRACE, "%s size=%dx%d", __func__, width, height);
  auto& internals = *(this->Internals);

  internals.EncodeCtx->bit_rate = this->BitRate;
  internals.EncodeCtx->width = this->Width;
  internals.EncodeCtx->height = this->Height;
  internals.EncodeCtx->time_base = AVRational{ this->TimeBaseStart, this->TimeBaseEnd };
  internals.EncodeCtx->framerate = AVRational{ this->TimeBaseEnd, this->TimeBaseStart };
  internals.EncodeCtx->gop_size = this->GroupOfPicturesSize;
  internals.EncodeCtx->max_b_frames = this->MaximumBFrames;
  switch (this->HWEncoderType)
  {
    case HardwareEncoderTypeEnum::VAAPI:
      internals.EncodeCtx->pix_fmt = AV_PIX_FMT_VAAPI;
      break;
    case HardwareEncoderTypeEnum::QSV:
      internals.EncodeCtx->pix_fmt = AV_PIX_FMT_QSV;
      break;
    case HardwareEncoderTypeEnum::NVENC:
      internals.EncodeCtx->pix_fmt = AV_PIX_FMT_CUDA;
      break;
    case HardwareEncoderTypeEnum::VideoToolbox:
      internals.EncodeCtx->pix_fmt = AV_PIX_FMT_VIDEOTOOLBOX;
      break;
    case HardwareEncoderTypeEnum::AMF:
      // TODO:
      // internals.EncodeCtx->pix_fmt = AV_PIX_FMT_AMF;
      break;
    case HardwareEncoderTypeEnum::MediaFoundation:
      // TODO:
      break;
    default:
      break;
  }
  internals.Tweak();

  if (!internals.SetupHWFrameCtx(internals.EncodeCtx->pix_fmt))
  {
    vtkLog(ERROR, << "Cannot use VAAPI for hardware accelerated encoding.");
    return false;
  }

  if (!internals.InitializeCodec())
  {
    vtkLog(ERROR, << "Could not open codec for encoding.");
    return false;
  }

  // Prepare a software frame.
  bool success = internals.InitializeSWFrame();

  // Setup a hardware frame.
  success &= internals.InitializeHWFrame();

  return success;
}

//------------------------------------------------------------------------------
void vtkFFmpegHardwareEncoder::TearDownEncoderFrame()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.TearDownEncoderFrames();
}

//------------------------------------------------------------------------------
bool vtkFFmpegHardwareEncoder::SupportsCodec(VTKVideoCodecType codec) const noexcept
{
  bool isSupported = false;
  switch (codec)
  {
    case VTKVideoCodecType::VTKVC_H264:
    case VTKVideoCodecType::VTKVC_H265:
    case VTKVideoCodecType::VTKVC_VP9:
      isSupported = true;
    case VTKVideoCodecType::VTKVC_AV1:
    default:
      break;
  }
  return isSupported;
}

//------------------------------------------------------------------------------
void vtkFFmpegHardwareEncoder::SetDeviceName(const char* dev)
{
  this->Device = dev == nullptr ? "" : dev;
  this->Modified();
  if (this->Initialized)
  {
    this->ShutdownInternal();
  }
}

//------------------------------------------------------------------------------
const char* vtkFFmpegHardwareEncoder::GetDeviceName()
{
  return this->Device.c_str();
}

//------------------------------------------------------------------------------
vtkIdType vtkFFmpegHardwareEncoder::GetLastEncodeTimeNS() const noexcept
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtEncode.count();
}

//------------------------------------------------------------------------------
vtkIdType vtkFFmpegHardwareEncoder::GetLastScaleTimeNS() const noexcept
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtScale.count();
}
