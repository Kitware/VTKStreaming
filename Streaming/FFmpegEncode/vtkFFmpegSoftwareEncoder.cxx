/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFmpegSoftwareEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFmpegSoftwareEncoder.h"
#include "vtkFFmpegEncoderInternals.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"

#include <chrono>
#include <cstddef>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/log.h>
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFFmpegSoftwareEncoder);

//------------------------------------------------------------------------------
vtkFFmpegSoftwareEncoder::vtkFFmpegSoftwareEncoder()
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
vtkFFmpegSoftwareEncoder::~vtkFFmpegSoftwareEncoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkFFmpegSoftwareEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

//------------------------------------------------------------------------------
std::string vtkFFmpegSoftwareEncoder::GetISOCodecParameterString() const noexcept
{
  return "unknown";
}

//------------------------------------------------------------------------------
bool vtkFFmpegSoftwareEncoder::InitializeInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);

  switch (this->Codec)
  {
    case VTKVideoCodecType::VTKVC_H264:
      internals.CodecName = "libx264";
      break;
    case VTKVideoCodecType::VTKVC_H265:
      internals.CodecName = "libx265";
      break;
    case VTKVideoCodecType::VTKVC_AV1:
      internals.CodecName = "libaom-av1";
      break;
    case VTKVideoCodecType::VTKVC_VP9:
    default:
      internals.CodecName = "libvpx-vp9";
      break;
  }

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

  return internals.InitializeBasicEncodeCtx();
}

//------------------------------------------------------------------------------
void vtkFFmpegSoftwareEncoder::ShutdownInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  // internals.Flush();
  internals.Shutdown();
}

//------------------------------------------------------------------------------
bool vtkFFmpegSoftwareEncoder::SetupEncoderFrame(int width, int height)
{
  vtkLogScopeF(TRACE, "%s size=%dx%d", __func__, width, height);
  auto& internals = *(this->Internals);

  internals.EncodeCtx->bit_rate = this->BitRate;
  internals.EncodeCtx->rc_max_rate = this->MaxBitRate;
  internals.EncodeCtx->rc_min_rate = this->MinBitRate;
  internals.EncodeCtx->qmax = this->QuantizationParameter;
  internals.EncodeCtx->thread_count = this->NumberOfEncoderThreads;
  internals.EncodeCtx->width = this->Width;
  internals.EncodeCtx->height = this->Height;
  internals.EncodeCtx->time_base = AVRational{ this->TimeBaseStart, this->TimeBaseEnd };
  internals.EncodeCtx->framerate = AVRational{ this->TimeBaseEnd, this->TimeBaseStart };
  internals.EncodeCtx->gop_size = this->GroupOfPicturesSize;
  internals.EncodeCtx->max_b_frames = this->MaximumBFrames;
  internals.EncodeCtx->pix_fmt = internals.InputPixFmt;
  internals.Tweak();

  if (!internals.InitializeCodec())
  {
    vtkLog(ERROR, << "Could not open codec for encoding.");
  }
  return internals.InitializeSWFrame();
}

//------------------------------------------------------------------------------
void vtkFFmpegSoftwareEncoder::TearDownEncoderFrame()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.TearDownEncoderFrames();
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkFFmpegSoftwareEncoder::SendEOS()
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
VTKVideoEncoderResultType vtkFFmpegSoftwareEncoder::EncodeInternal(
  vtkSmartPointer<vtkRawVideoFrame> frame)
{
  vtkLogScopeFunction(TRACE);
  VTKVideoEncoderResultType result;
  auto& internals = *(this->Internals);
  const int64_t pts = (internals.SendCounter++ % this->TimeBaseEnd) + 1;
  internals.SoftwareFrame->pts = pts ? pts : this->TimeBaseEnd;

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
bool vtkFFmpegSoftwareEncoder::SupportsCodec(VTKVideoCodecType vtkNotUsed(codec)) const noexcept
{
  return true;
}

//------------------------------------------------------------------------------
vtkIdType vtkFFmpegSoftwareEncoder::GetLastEncodeTimeNS() const noexcept
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtEncode.count();
}

//------------------------------------------------------------------------------
vtkIdType vtkFFmpegSoftwareEncoder::GetLastScaleTimeNS() const noexcept
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtScale.count();
}
