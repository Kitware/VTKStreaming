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
#include "vtkFFMPEGEncoderInternals.h"
#include "vtkRawVideoFrame.h"

#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include <chrono>
#include <cstddef>

extern "C"
{
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
bool vtkFFMPEGSoftwareEncoder::PushInternal(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.SoftwareFrame->pts = frame->GetPresentationTS();

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
bool vtkFFMPEGSoftwareEncoder::SetupEncoderFrame(const int& w, const int& h)
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
bool vtkFFMPEGSoftwareEncoder::Encode()
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
bool vtkFFMPEGSoftwareEncoder::IsCodecSupported(VTKCodecType codec)
{
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
