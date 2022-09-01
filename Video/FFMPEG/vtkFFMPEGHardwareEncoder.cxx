/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGHardwareEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFMPEGHardwareEncoder.h"
#include "vtkCodedVideoPacket.h"
#include "vtkFFMPEGEncoderInternals.h"
#include "vtkRawVideoFrame.h"

#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include "vtksys/SystemInformation.hxx"

extern "C"
{
#include <libavutil/hwcontext.h>
#include <libavutil/log.h>
#include <libavutil/pixfmt.h>
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFFMPEGHardwareEncoder);

//------------------------------------------------------------------------------
vtkFFMPEGHardwareEncoder::vtkFFMPEGHardwareEncoder()
  : Internals(new vtkFFMPEGEncoderInternals())
{
#ifndef NDEBUG
  av_log_set_level(AV_LOG_TRACE);
#endif
}

//------------------------------------------------------------------------------
vtkFFMPEGHardwareEncoder::~vtkFFMPEGHardwareEncoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkFFMPEGHardwareEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

//------------------------------------------------------------------------------
bool vtkFFMPEGHardwareEncoder::QueryPlatformSupport()
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
      this->HWEncoderType = HardwareEncoderTypeEnum::QSV; // TODO: find proper tweaks to get qsv
                                                          // working on linux and windows.
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
void vtkFFMPEGHardwareEncoder::ClearGPUPreference()
{
  vtkLogScopeFunction(TRACE);
  this->PreferredGPU = DesktopGPUVendor::None;
}

//------------------------------------------------------------------------------
void vtkFFMPEGHardwareEncoder::PreferAMDEncoders()
{
  vtkLogScopeFunction(TRACE);
  this->PreferredGPU = DesktopGPUVendor::AMD;
}

//------------------------------------------------------------------------------
void vtkFFMPEGHardwareEncoder::PreferNVIDIAEncoders()
{
  vtkLogScopeFunction(TRACE);
  this->PreferredGPU = DesktopGPUVendor::NVIDIA;
}

//------------------------------------------------------------------------------
void vtkFFMPEGHardwareEncoder::PreferIntelEncoders()
{
  vtkLogScopeFunction(TRACE);
  this->PreferredGPU = DesktopGPUVendor::Intel;
}

//------------------------------------------------------------------------------
bool vtkFFMPEGHardwareEncoder::InitializeInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  if (!this->QueryPlatformSupport())
  {
    vtkLog(ERROR, << "Failed to find a hardware encoder suitable for your OS and GPU.");
    return false;
  }

  bool success = true;
  switch (this->HWEncoderType)
  {
    case HardwareEncoderTypeEnum::VAAPI:
      switch (this->Codec)
      {
        case H264:
          internals.CodecName = "h264_vaapi";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_VAAPI);
          break;
        case H265:
          internals.CodecName = "hevc_vaapi";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_VAAPI);
          break;
        case AV1:
          vtkLog(ERROR, << "AV1 is not supported by VAAPI hardware encoder");
          success = false;
          break;
        case VP9:
        default:
          internals.CodecName = "vp9_vaapi";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_VAAPI);
          break;
      }
      break;
    case HardwareEncoderTypeEnum::QSV:
      switch (this->Codec)
      {
        case H264:
          internals.CodecName = "h264_qsv";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_QSV);
          break;
        case H265:
          internals.CodecName = "hevc_qsv";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_QSV);
          break;
        case AV1:
          vtkLog(ERROR, << "AV1 is not supported by QSV hardware encoder");
          success = false;
          break;
        case VP9:
        default:
          internals.CodecName = "vp9_qsv";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_QSV);
          break;
      }
      break;
    case HardwareEncoderTypeEnum::AMF:
      switch (this->Codec)
      {
        case H264:
          internals.CodecName = "h264_amf";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = false; // TODO: Confirm on AMD gpu.
          break;
        case H265:
          internals.CodecName = "hevc_amf";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = false; // TODO: Confirm on AMD gpu.
          break;
        case AV1:
          vtkLog(ERROR, << "AV1 is not supported by AMF hardware encoder");
          success = false;
          break;
        case VP9:
        default:
          vtkLog(ERROR, << "VP9 is not supported by AMF hardware encoder");
          success = false;
          break;
      }
      break;
    case HardwareEncoderTypeEnum::NVENC:
      switch (this->Codec)
      {
        case H264:
          internals.CodecName = "h264_nvenc";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_CUDA);
          break;
        case H265:
          internals.CodecName = "hevc_nvenc";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_CUDA);
          break;
        case AV1:
          vtkLog(ERROR, << "AV1 is not supported by NVENC hardware encoder");
          success = false;
          break;
        case VP9:
        default:
          vtkLog(ERROR, << "VP9 is not supported by NVENC hardware encoder");
          success = false;
          break;
      }
      break;
    case HardwareEncoderTypeEnum::VideoToolbox:
      switch (this->Codec)
      {
        case H264:
          internals.CodecName = "h264_videotoolbox";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
          success = false; // TODO: Confirm on mac.
          break;
        case H265:
          internals.CodecName = "hevc_videotoolbox";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
          success = false; // TODO: Confirm on mac.
          break;
        case AV1:
          vtkLog(ERROR, << "AV1 is not supported by VideoToolbox hardware encoder");
          success = false;
          break;
        case VP9:
        default:
          vtkLog(ERROR, << "VP9 is not supported by VideoToolbox hardware encoder");
          success = false; // TODO: Confirm on mac.
          break;
      }
      break;
    case HardwareEncoderTypeEnum::MediaFoundation:
      switch (this->Codec)
      {
        case H264:
          internals.CodecName = "h264_mediafoundation";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = false; // TODO: Confirm on windows.
          break;
        case H265:
          internals.CodecName = "hevc_mediafoundation";
          internals.InputPixFmt = AV_PIX_FMT_NV12;
          success = false; // TODO: Confirm on windows.
          break;
        case AV1:
          vtkLog(ERROR, << "AV1 is not supported by MediaFoundation encoder");
          success = false;
          break;
        case VP9:
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
void vtkFFMPEGHardwareEncoder::ShutdownInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.Shutdown();
}

//------------------------------------------------------------------------------
void vtkFFMPEGHardwareEncoder::FlushInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.Flush();
}

//------------------------------------------------------------------------------
bool vtkFFMPEGHardwareEncoder::PushInternal(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.SoftwareFrame->pts = frame->GetPresentationTS();
  internals.HardwareFrame->pts = frame->GetPresentationTS();

  auto tStart = std::chrono::high_resolution_clock::now();
  bool preprocSuccess = internals.PreprocessInput(frame);
  internals.dtScale = std::chrono::high_resolution_clock::now() - tStart;
  if (!preprocSuccess)
  {
    vtkLog(ERROR, << "Failed to convert rgba32 to encoder input frame pixel format.");
  }

  return this->Encode();
}

//------------------------------------------------------------------------------
bool vtkFFMPEGHardwareEncoder::SetupEncoderFrame(const int& w, const int& h)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);

  internals.EncodeCtx->bit_rate = this->BitRate;
  internals.EncodeCtx->width = w;
  internals.EncodeCtx->height = h;
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
  }

  // Prepare a software frame.
  bool success = internals.InitializeSWFrame();

  // Setup a hardware frame.
  success &= internals.InitializeHWFrame();
  return success;
}

//------------------------------------------------------------------------------
bool vtkFFMPEGHardwareEncoder::NeedsNewEncoderFrame(const int& w, const int& h)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  vtkLogScopeFunction(TRACE);
  return w != internals.LastEncodedFrameDims[0] && h != internals.LastEncodedFrameDims[1];
}

//------------------------------------------------------------------------------
void vtkFFMPEGHardwareEncoder::TearDownEncoderFrame()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.TearDownEncoderFrames();
}

//------------------------------------------------------------------------------
bool vtkFFMPEGHardwareEncoder::Encode()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  vtkFFMPEGEncoderInternals::PacketRecvCallbackT packetReciever = [this](vtkCodedVideoPacket* pkt)
  { this->PacketHandler(pkt); };

  bool success = internals.Encode(this->GetForceIFrame(), packetReciever);
  if (!success)
  {
    vtkLog(ERROR, << "Failed to encode.");
  }
  return success;
}

//------------------------------------------------------------------------------
bool vtkFFMPEGHardwareEncoder::IsCodecSupported(VTKCodecType codec)
{
  bool isSupported = false;
  switch (codec)
  {
    case H264:
    case H265:
    case VP9:
      isSupported = true;
    case AV1:
    default:
      break;
  }
  return isSupported;
}

//------------------------------------------------------------------------------
void vtkFFMPEGHardwareEncoder::SetDeviceName(const char* dev)
{
  this->Device = dev == nullptr ? "" : dev;
  this->Modified();
  if (this->Initialized)
  {
    this->ShutdownInternal();
  }
}

//------------------------------------------------------------------------------
const char* vtkFFMPEGHardwareEncoder::GetDeviceName()
{
  return this->Device.c_str();
}

//------------------------------------------------------------------------------
vtkIdType vtkFFMPEGHardwareEncoder::GetLastEncodeTimeNS()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtEncode.count();
}

//------------------------------------------------------------------------------
vtkIdType vtkFFMPEGHardwareEncoder::GetLastScaleTimeNS()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtScale.count();
}
