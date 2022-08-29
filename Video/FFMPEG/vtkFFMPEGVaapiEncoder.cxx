/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGVaapiEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFMPEGVaapiEncoder.h"
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

extern "C"
{
#include <libavutil/log.h>
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFFMPEGVaapiEncoder);

//------------------------------------------------------------------------------
vtkFFMPEGVaapiEncoder::vtkFFMPEGVaapiEncoder()
  : Internals(new vtkFFMPEGEncoderInternals())
{
#ifndef NDEBUG
  av_log_set_level(AV_LOG_TRACE);
#endif
}

//------------------------------------------------------------------------------
vtkFFMPEGVaapiEncoder::~vtkFFMPEGVaapiEncoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkFFMPEGVaapiEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

//------------------------------------------------------------------------------
bool vtkFFMPEGVaapiEncoder::InitializeInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);

  switch (this->Codec)
  {
    case HEVC:
      internals.CodecName = "h265_vaapi";
      internals.InputPixFmt = AV_PIX_FMT_NV12;
      break;
    case JPEG:
      vtkLog(ERROR, << "JPEG is not supported by this encoder");
      break;
    case BMP:
      vtkLog(ERROR, << "BMP is not supported by this encoder");
      break;
    case AV1:
      vtkLog(ERROR, << "AV1 is not supported by this encoder");
      break;
    case VP9:
    default:
      internals.CodecName = "vp9_vaapi";
      internals.InputPixFmt = AV_PIX_FMT_NV12;
      break;
  }

  return internals.InitializeHWEncodeCtx(AV_HWDEVICE_TYPE_VAAPI);
}

//------------------------------------------------------------------------------
void vtkFFMPEGVaapiEncoder::ShutdownInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.Shutdown();
}

//------------------------------------------------------------------------------
void vtkFFMPEGVaapiEncoder::FlushInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.Flush();
}

//------------------------------------------------------------------------------
bool vtkFFMPEGVaapiEncoder::PushInternal(vtkRawVideoFrame* frame)
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
bool vtkFFMPEGVaapiEncoder::SetupEncoderFrame(const int& w, const int& h)
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
  internals.EncodeCtx->pix_fmt = AV_PIX_FMT_VAAPI;
  internals.Tweak();

  if (!internals.SetupVAAPIHWFrameCtx())
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
bool vtkFFMPEGVaapiEncoder::NeedsNewEncoderFrame(const int& w, const int& h)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  vtkLogScopeFunction(TRACE);
  return w != internals.LastEncodedFrameDims[0] && h != internals.LastEncodedFrameDims[1];
}

//------------------------------------------------------------------------------
void vtkFFMPEGVaapiEncoder::TearDownEncoderFrame()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  internals.TearDownEncoderFrames();
}

//------------------------------------------------------------------------------
bool vtkFFMPEGVaapiEncoder::Encode()
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
bool vtkFFMPEGVaapiEncoder::IsCodecSupported(VTKCodecType codec)
{
  bool isSupported = false;
  switch (codec)
  {
    case HEVC:
    case VP9:
      isSupported = true;
    case JPEG:
    case BMP:
    case AV1:
    default:
      break;
  }
  return isSupported;
}

//------------------------------------------------------------------------------
void vtkFFMPEGVaapiEncoder::SetDeviceName(const char* dev)
{
  this->Device = dev == nullptr ? "" : dev;
  this->Modified();
  if (this->Initialized)
  {
    this->ShutdownInternal();
  }
}

//------------------------------------------------------------------------------
const char* vtkFFMPEGVaapiEncoder::GetDeviceName()
{
  return this->Device.c_str();
}

//------------------------------------------------------------------------------
vtkIdType vtkFFMPEGVaapiEncoder::GetLastEncodeTimeNS()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtEncode.count();
}

//------------------------------------------------------------------------------
vtkIdType vtkFFMPEGVaapiEncoder::GetLastScaleTimeNS()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtScale.count();
}
