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
#include "vtkFFMPEGCommon.h"
#include "vtkFFMPEGDecoderInternals.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"

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
    case H264:
      internals.CodecName = "h264";
      break;
    case H265:
      internals.CodecName = "hevc";
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
    avcodec_flush_buffers(internals.DecodeCtx);
    avcodec_free_context(&internals.DecodeCtx);
    internals.DecodeCtx = nullptr;
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
void vtkFFMPEGSoftwareDecoder::DrainInternal() {}

//------------------------------------------------------------------------------
DecoderResultType vtkFFMPEGSoftwareDecoder::DecodeInternal(vtkCodedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  DecoderResultType result;
  auto& internals = *(this->Internals);
  if (!internals.Packet)
  {
    vtkLog(ERROR, << "Internal packet object is null. Cannot send packet for decoding!");
    result.first = VTKVideoProcessingStatusType::InvalidValue;
    result.second = nullptr;
    return result;
  }

  internals.Packet->size = packet->GetData(internals.Packet->data);
  // at the moment, since there's only one ffmpeg decoder implementation (software),
  // i'm not writing the send/recv sections of code in the internals class.
  int statusCode = avcodec_send_packet(internals.DecodeCtx, internals.Packet);
  auto status = ParseFFMPEGStatus(statusCode, /*during_send*/ true);
  if (statusCode < 0)
  {
    vtkLog(ERROR,
      "Error sending a packet for decoding. Error - " << VTKVideoProcessingStatusTypeToStr(status)
                                                      << " (" << statusCode << ")");
    result.first = status;
    result.second = nullptr;
    return result;
  }

  if (!internals.SoftwareFrame)
  {
    vtkLog(ERROR, << "Frame is null. Cannot decode!");
    result.first = VTKVideoProcessingStatusType::UnknownError; // dunno why frame is null.
    result.second = nullptr;
    return result;
  }

  auto tStart = std::chrono::high_resolution_clock::now();
  statusCode = avcodec_receive_frame(internals.DecodeCtx, internals.SoftwareFrame);
  auto now = std::chrono::high_resolution_clock::now();

  if (statusCode < 0)
  {
    vtkLog(ERROR,
      "Failed to receive uncompressed image from decoder. Error - "
        << VTKVideoProcessingStatusTypeToStr(status) << " (" << statusCode << ")");
    result.first = status;
    result.second = nullptr;
    return result;
  }
  else
  {
    internals.dtDecode = now - tStart;
    result.first = status;
    result.second.TakeReference(internals.GetOutputFrameFromDecodedFrame());
    vtkLog(TRACE, << "Successfully decoded image - "
                  << result.second->GetSize(0) + result.second->GetSize(1)
                  << result.second->GetSize(2) << " bytes");
    return result;
  }
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
