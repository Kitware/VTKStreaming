/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGSoftwareDecoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFMPEGSoftwareDecoder.h"
#include "vtkCodedVideoPacket.h"
#include "vtkFFMPEGDecoderInternals.h"
#include "vtkRawVideoFrame.h"

#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPixelFormats.h"
#include "vtkUnsignedCharArray.h"

extern "C"
{
#include <libavutil/log.h>
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFFMPEGSoftwareDecoder);

//------------------------------------------------------------------------------
vtkFFMPEGSoftwareDecoder::vtkFFMPEGSoftwareDecoder()
  : Internals(new vtkFFMPEGDecoderInternals())
{
#ifndef NDEBUG
  av_log_set_level(AV_LOG_VERBOSE);
#endif
}

//------------------------------------------------------------------------------
vtkFFMPEGSoftwareDecoder::~vtkFFMPEGSoftwareDecoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkFFMPEGSoftwareDecoder::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

//------------------------------------------------------------------------------
bool vtkFFMPEGSoftwareDecoder::InitializeInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);

  switch (this->CodecType)
  {
    case HEVC:
      internals.CodecName = "hevc";
      break;
    case JPEG:
      internals.CodecName = "libopenjpeg";
      break;
    case BMP:
      internals.CodecName = "bmp";
      break;
    case AV1:
      internals.CodecName = "libaom-av1";
      break;
    case VP9:
    default:
      internals.CodecName = "libvpx-vp9";
      break;
  }

  internals.Codec = avcodec_find_decoder_by_name(internals.CodecName.c_str());
  if (!internals.Codec)
  {
    vtkLog(ERROR, "Codec " << internals.CodecName << " not found");
    return false;
  }

  vtkLog(INFO, << "Codec description");
  vtkLog(INFO, << "Name " << internals.Codec->name);
  vtkLog(INFO, << "Long name " << internals.Codec->long_name);
  vtkLog(INFO, << "Wrapper name " << internals.Codec->wrapper_name);

  internals.DecodeCtx = avcodec_alloc_context3(internals.Codec);
  if (!internals.DecodeCtx)
  {
    vtkLog(ERROR, << "Failed to allocate a decode context.");
    return false;
  }

  if (avcodec_open2(internals.DecodeCtx, internals.Codec, nullptr) < 0)
  {
    vtkLog(ERROR, "Could not open codec");
    return false;
  }

  internals.Packet = av_packet_alloc();
  if (!internals.Packet)
  {
    vtkLog(ERROR, << "Failed to allocate an AVPacket");
    return false;
  }

  internals.SoftwareFrame = av_frame_alloc();
  if (!internals.SoftwareFrame)
  {
    vtkLog(ERROR, "Could not allocate decoder output frame");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkFFMPEGSoftwareDecoder::ShutdownInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);

  if (internals.DecodeCtx)
  {
    avcodec_free_context(&internals.DecodeCtx);
    internals.DecodeCtx = nullptr;
  }

  if (internals.OutputAVFrame)
  {
    av_frame_free(&internals.OutputAVFrame);
    internals.OutputAVFrame = nullptr;
  }

  if (internals.SoftwareFrame)
  {
    av_frame_free(&internals.SoftwareFrame);
    internals.SoftwareFrame = nullptr;
  }

  if (internals.Packet)
  {
    av_packet_free(&internals.Packet);
    internals.Packet = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkFFMPEGSoftwareDecoder::FlushInternal() {}

//------------------------------------------------------------------------------
bool vtkFFMPEGSoftwareDecoder::PushInternal(vtkCodedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);

  internals.Packet->size = packet->GetData(internals.Packet->data);
  bool success = this->Decode();
  return success;
}

//------------------------------------------------------------------------------
// TODO: Think of a way to not let this function block the calling thread.
bool vtkFFMPEGSoftwareDecoder::Decode()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);

  if (!internals.SoftwareFrame)
  {
    vtkLog(ERROR, << "Frame is null. Cannot decode!");
    return false;
  }

  auto tStart = std::chrono::high_resolution_clock::now();
  int ret = avcodec_send_packet(internals.DecodeCtx, internals.Packet);
  if (ret < 0)
  {
    vtkLog(ERROR, "Error sending a packet for decoding");
    return false;
  }

  while (ret >= 0)
  {
    ret = avcodec_receive_frame(internals.DecodeCtx, internals.SoftwareFrame);
    auto now = std::chrono::high_resolution_clock::now();
    internals.dtDecode = now - tStart;
    tStart = now;
    switch (ret)
    {
      case AVERROR(EAGAIN):
      {
        vtkLog(TRACE, << "Decoder needs more input packets");
        return true;
      }
      case AVERROR_EOF:
      {
        vtkLog(TRACE, << "Decoder input is flushed. No more output frames.");
        return true;
      }
      case AVERROR(EINVAL):
      {
        vtkLog(ERROR, << "Codec is not opened, or the context is a decoding context.");
        break;
      }
      default:
        break;
    }
    if (ret < 0)
    {
      vtkLog(ERROR, "Error during decoding");
      return false;
    }
    else
    {
      switch (this->OutputPixelFormat)
      {
        case YUV420P:
          internals.InitializeOutputFrame(AV_PIX_FMT_YUV420P);
          internals.OutputVideoFrame->SetSliceOrder(vtkRawVideoFrame::SliceOrderType::TopDown);
          break;
        case RGBA32:
        default:
          internals.InitializeOutputFrame(AV_PIX_FMT_RGBA);
          internals.OutputVideoFrame->SetSliceOrder(vtkRawVideoFrame::SliceOrderType::BottomUp);
          break;
      }
      internals.GetOutputFrameFromDecodedFrame();
      this->InvokeEvent(vtkCommand::ProgressEvent, internals.OutputVideoFrame);
    }
  }
  return true;
}

//------------------------------------------------------------------------------
vtkIdType vtkFFMPEGSoftwareDecoder::GetLastDecodeTimeNS()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtDecode.count();
}

//------------------------------------------------------------------------------
vtkIdType vtkFFMPEGSoftwareDecoder::GetLastScaleTimeNS()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtScale.count();
}
