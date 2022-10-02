/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFmpegSoftwareDecoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFmpegSoftwareDecoder.h"
#include "vtkFFmpegDecoderInternals.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkSmartPointer.h"

extern "C"
{
#include <libavutil/log.h>
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFFmpegSoftwareDecoder);

//------------------------------------------------------------------------------
vtkFFmpegSoftwareDecoder::vtkFFmpegSoftwareDecoder()
  : Internals(new vtkFFmpegDecoderInternals())
{
#ifndef NDEBUG
  av_log_set_level(AV_LOG_VERBOSE);
#endif
}

//------------------------------------------------------------------------------
vtkFFmpegSoftwareDecoder::~vtkFFmpegSoftwareDecoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkFFmpegSoftwareDecoder::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

//------------------------------------------------------------------------------
bool vtkFFmpegSoftwareDecoder::InitializeInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);

  switch (this->Codec)
  {
    case VTKVideoCodecType::VTKVC_H264:
      internals.CodecName = "h264";
      break;
    case VTKVideoCodecType::VTKVC_H265:
      internals.CodecName = "hevc";
      break;
    case VTKVideoCodecType::VTKVC_AV1:
      internals.CodecName = "libaom-av1";
      break;
    case VTKVideoCodecType::VTKVC_VP9:
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
void vtkFFmpegSoftwareDecoder::ShutdownInternal()
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
void vtkFFmpegSoftwareDecoder::FlushInternal()
{
  vtkLogScopeFunction(TRACE);
  this->DrainInternal();
  auto& internals = *(this->Internals);
  avcodec_flush_buffers(internals.DecodeCtx);
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkFFmpegSoftwareDecoder::DrainInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  VTKVideoDecoderResultType result;
  int ret = avcodec_send_packet(internals.DecodeCtx, nullptr);
  while (ret >= 0)
  {
    ret = avcodec_receive_frame(internals.DecodeCtx, internals.SoftwareFrame);
    switch (ret)
    {
      case AVERROR(EAGAIN):
      case AVERROR(EINVAL):
      default:
        break;
      case AVERROR_EOF:
        vtkLog(TRACE, "Draining complete");
        ret = -1;
        break;
    }
  }
  return result;
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkFFmpegSoftwareDecoder::PushInternal(
  vtkCompressedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  if (!internals.Packet)
  {
    vtkLog(ERROR, << "Internal packet object is null. Cannot send packet for decoding!");
    return VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
  }

  internals.Packet->size = packet->GetData(internals.Packet->data);
  // at the moment, since there's only one ffmpeg decoder implementation (software),
  // i'm not writing the send/recv sections of code in the internals class.
  int statusCode = avcodec_send_packet(internals.DecodeCtx, internals.Packet);
  auto status = vtkFFmpegDecoderInternals::ParseFFMPEGStatus(statusCode, /*during_send*/ true);
  return status;
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkFFmpegSoftwareDecoder::GetResultInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  VTKVideoDecoderResultType result;
  if (!internals.SoftwareFrame)
  {
    vtkLog(ERROR, << "Frame is null. Cannot decode!");
    result.first =
      VTKVideoProcessingStatusType::VTKVPStatus_UnknownError; // dunno why frame is null.
    result.second = {};
    return result;
  }
  auto statusCode = avcodec_receive_frame(internals.DecodeCtx, internals.SoftwareFrame);
  auto status = vtkFFmpegDecoderInternals::ParseFFMPEGStatus(statusCode, /*during_send*/ false);
  if (statusCode < 0)
  {
    vtkLog(ERROR,
      "Failed to receive uncompressed image from decoder. Error - "
        << vtkVideoProcessingStatusTypeUtilities::ToString(status) << " (" << statusCode << ")");
    result.first = status;
    result.second = {};
    return result;
  }
  else
  {
    result.first = status;
    result.second.emplace_back(
      vtk::TakeSmartPointer(internals.GetOutputFrameFromDecodedFrame(this->GraphicsContext)));
    vtkLogF(TRACE, "Successfully decoded image - %d bytes", result.second[0]->GetActualSize());
    return result;
  }
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkFFmpegSoftwareDecoder::DecodeInternal(vtkCompressedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  VTKVideoDecoderResultType result;
  auto& internals = *(this->Internals);
  if (!internals.Packet)
  {
    vtkLog(ERROR, << "Internal packet object is null. Cannot send packet for decoding!");
    result.first = VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
    result.second = {};
    return result;
  }

  auto tStart = std::chrono::high_resolution_clock::now();
  internals.Packet->size = packet->GetData(internals.Packet->data);
  // at the moment, since there's only one ffmpeg decoder implementation (software),
  // i'm not writing the send/recv sections of code in the internals class.
  int statusCode = avcodec_send_packet(internals.DecodeCtx, internals.Packet);
  auto status = vtkFFmpegDecoderInternals::ParseFFMPEGStatus(statusCode, /*during_send*/ true);
  if (statusCode < 0)
  {
    vtkLog(ERROR,
      "Error sending a packet for decoding. Error - "
        << vtkVideoProcessingStatusTypeUtilities::ToString(status) << " (" << statusCode << ")");
    result.first = status;
    result.second = {};
    return result;
  }

  if (!internals.SoftwareFrame)
  {
    vtkLog(ERROR, << "Frame is null. Cannot decode!");
    result.first =
      VTKVideoProcessingStatusType::VTKVPStatus_UnknownError; // dunno why frame is null.
    result.second = {};
    return result;
  }

  statusCode = avcodec_receive_frame(internals.DecodeCtx, internals.SoftwareFrame);
  status = vtkFFmpegDecoderInternals::ParseFFMPEGStatus(statusCode, /*during_send*/ false);
  auto now = std::chrono::high_resolution_clock::now();

  if (statusCode < 0)
  {
    vtkLog(ERROR,
      "Failed to receive uncompressed image from decoder. Error - "
        << vtkVideoProcessingStatusTypeUtilities::ToString(status) << " (" << statusCode << ")");
    result.first = status;
    result.second = {};
    return result;
  }
  else
  {
    internals.dtDecode = now - tStart;
    result.first = status;
    result.second.emplace_back(
      vtk::TakeSmartPointer(internals.GetOutputFrameFromDecodedFrame(this->GraphicsContext)));
    vtkLogF(TRACE, "Successfully decoded image - %d bytes", result.second[0]->GetActualSize());
    return result;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkFFmpegSoftwareDecoder::GetLastDecodeTimeNS() const noexcept
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtDecode.count();
}

//------------------------------------------------------------------------------
vtkIdType vtkFFmpegSoftwareDecoder::GetLastScaleTimeNS() const noexcept
{
  vtkLogScopeFunction(TRACE);
  auto& internals = *(this->Internals);
  return internals.dtScale.count();
}

//------------------------------------------------------------------------------
bool vtkFFmpegSoftwareDecoder::SupportsCodec(VTKVideoCodecType vtkNotUsed(codec)) const noexcept
{
  vtkLogScopeFunction(TRACE);
  return true;
}
