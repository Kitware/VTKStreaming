/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGSoftwareEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFMPEGSoftwareEncoder.h"
#include "vtkCodedVideoPacket.h"
#include "vtkFFMPEGCommon.h"
#include "vtkFFMPEGEncoderInternals.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkRawVideoFrame.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <chrono>
#include <cstddef>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/log.h>
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFFMPEGSoftwareEncoder);

//------------------------------------------------------------------------------
vtkFFMPEGSoftwareEncoder::vtkFFMPEGSoftwareEncoder()
  : Internals(new vtkFFMPEGEncoderInternals())
{
#ifndef NDEBUG
  av_log_set_level(AV_LOG_TRACE);
#endif
}

//------------------------------------------------------------------------------
vtkFFMPEGSoftwareEncoder::~vtkFFMPEGSoftwareEncoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkFFMPEGSoftwareEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

//------------------------------------------------------------------------------
bool vtkFFMPEGSoftwareEncoder::InitializeInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);

  switch (this->Codec)
  {
    case H264:
      internals.CodecName = "libx264";
      internals.InputPixFmt = AV_PIX_FMT_YUV420P;
      break;
    case H265:
      internals.CodecName = "libx265";
      internals.InputPixFmt = AV_PIX_FMT_YUV420P;
      break;
    case AV1:
      internals.CodecName = "libaom-av1";
      internals.CodecName = "libsvtav1";
      internals.InputPixFmt = AV_PIX_FMT_YUV420P;
      break;
    case VP9:
    default:
      internals.CodecName = "libvpx-vp9";
      internals.InputPixFmt = AV_PIX_FMT_YUV420P;
      break;
  }

  return internals.InitializeBasicEncodeCtx();
}

//------------------------------------------------------------------------------
void vtkFFMPEGSoftwareEncoder::ShutdownInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.Shutdown();
}

//------------------------------------------------------------------------------
void vtkFFMPEGSoftwareEncoder::FlushInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.Flush();
}

//------------------------------------------------------------------------------
bool vtkFFMPEGSoftwareEncoder::SetupEncoderFrame(const int& width, const int& height)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);

  internals.EncodeCtx->bit_rate = this->BitRate;
  internals.EncodeCtx->rc_max_rate = this->MaxBitRate;
  internals.EncodeCtx->rc_min_rate = this->MinBitRate;
  internals.EncodeCtx->thread_count = this->NumberOfEncoderThreads;
  internals.EncodeCtx->width = width;
  internals.EncodeCtx->height = height;
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
bool vtkFFMPEGSoftwareEncoder::NeedsNewEncoderFrame(const int& w, const int& h)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  vtkLogScopeFunction(TRACE);
  return w != internals.LastEncodedFrameDims[0] && h != internals.LastEncodedFrameDims[1];
}

//------------------------------------------------------------------------------
void vtkFFMPEGSoftwareEncoder::TearDownEncoderFrame()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.TearDownEncoderFrames();
}

//------------------------------------------------------------------------------
void vtkFFMPEGSoftwareEncoder::DrainInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  avcodec_send_frame(internals.EncodeCtx, nullptr);
}

//------------------------------------------------------------------------------
EncoderResultType vtkFFMPEGSoftwareEncoder::EncodeInternal(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  EncoderResultType result;
  auto& internals = *(this->Internals);
  const int64_t pts = frame->GetPresentationTS() % this->TimeBaseEnd;
  internals.SoftwareFrame->pts = pts ? pts : this->TimeBaseEnd;

  if (!internals.PreprocessInput(frame))
  {
    vtkLog(ERROR, << "Failed to convert rgba32 to encoder input frame pixel format.");
    result.first = VTKVideoProcessingStatusType::InvalidValue;
    result.second = nullptr;
    return result;
  }

  if (!internals.PrepareForEncoding())
  {
    result.first = VTKVideoProcessingStatusType::InvalidValue;
    result.second = nullptr;
    return result;
  }

  return internals.Encode(this->GetForceIFrame());
}

//------------------------------------------------------------------------------
bool vtkFFMPEGSoftwareEncoder::IsCodecSupported(VTKCodecType codec)
{
  (void)codec;
  return true;
}

//------------------------------------------------------------------------------
vtkIdType vtkFFMPEGSoftwareEncoder::GetLastEncodeTimeNS()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtEncode.count();
}

//------------------------------------------------------------------------------
vtkIdType vtkFFMPEGSoftwareEncoder::GetLastScaleTimeNS()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtScale.count();
}
