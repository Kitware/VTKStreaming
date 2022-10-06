/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFmpegEncoderInternals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFmpegEncoderInternals.h"
#include "vtkCompressedVideoPacket.h"
#include "vtkLogger.h"
#include "vtkRawVideoFrame.h"
#include "vtkSmartPointer.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <chrono>
#include <cstdint>

extern "C"
{
#include <libavutil/dict.h>
#include <libavutil/pixfmt.h>
}

//------------------------------------------------------------------------------
bool vtkFFmpegEncoderInternals::ConvertRGBA32ToEncoderPixFmt(vtkRawVideoFrame* rgba32Image)
{
  vtkLogScopeFunction(TRACE);

  const int& srcW = rgba32Image->GetStorageWidth();
  const int& srcH = rgba32Image->GetStorageHeight();
  const int& dstW = this->SoftwareFrame->width;
  const int& dstH = this->SoftwareFrame->height;

  vtkLog(TRACE, "Scale " << srcW << 'x' << srcH << "->" << dstW << 'x' << dstH);

  auto array = rgba32Image->GetData();
  auto dptr = array->GetPointer(0);

  this->SwScaleCtx = sws_getCachedContext(this->SwScaleCtx, srcW, srcH, AV_PIX_FMT_RGBA, dstW, dstH,
    this->InputPixFmt, 0, nullptr, nullptr, nullptr);
  if (!this->SwScaleCtx)
  {
    vtkLog(ERROR, << "Could not initialize a scaling context for given parameters");
    return false;
  }
  if (av_frame_make_writable(this->SoftwareFrame) < 0)
  {
    vtkLog(ERROR, "Failed to make frame writable");
    return false;
  }
  // ffMPEG requires top-down ordering. if slice order is bottomup, convert it.
  int sign = 1;
  if (rgba32Image->GetSliceOrderType() == vtkRawVideoFrame::SliceOrderType::BottomUp)
  {
    sign = -1;
    dptr += static_cast<ptrdiff_t>(4 * srcW * (srcH - 1));
  }
  // sws_scale requires a full const cast.
  auto rgba32 = (const uint8_t* const*)&dptr;
  // r, g, b, a -> four components, total size = 4 * w * h, linesize = total_size/h
  const int inLinesize[1] = { sign * 4 * srcW };
  sws_scale(this->SwScaleCtx, rgba32, inLinesize, 0, srcH, this->SoftwareFrame->data,
    this->SoftwareFrame->linesize);
  return true;
}

//------------------------------------------------------------------------------
bool vtkFFmpegEncoderInternals::PrepareForEncoding()
{
  vtkLogScopeFunction(TRACE);
  if (this->SoftwareFrame == nullptr)
  {
    vtkLog(ERROR, << "Frame is null. Cannot prepare for encode!");
    return false;
  }
  if (this->HardwareFrame != nullptr)
  {
    if (av_hwframe_transfer_data(this->HardwareFrame, this->SoftwareFrame, 0) < 0)
    {
      vtkLog(ERROR, << "Error while transferring frame data to hardware surface.");
      return false;
    }
    this->Frame = this->HardwareFrame;
  }
  else
  {
    this->Frame = this->SoftwareFrame;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkFFmpegEncoderInternals::InitializeHWEncodeCtx(AVHWDeviceType type)
{
  vtkLogScopeFunction(TRACE);
  if (av_hwdevice_ctx_create(&this->HardwareDevCtx, type, nullptr, nullptr, 0) < 0)
  {
    vtkLog(ERROR, << "Failed to create a HW encoding context.");
    return false;
  }

  return this->InitializeBasicEncodeCtx();
}

//------------------------------------------------------------------------------
bool vtkFFmpegEncoderInternals::InitializeBasicEncodeCtx()
{
  vtkLogScopeFunction(TRACE);
  this->Codec = avcodec_find_encoder_by_name(this->CodecName.c_str());
  if (!this->Codec)
  {
    vtkLog(ERROR, "Codec " << this->CodecName << " not found");
    return false;
  }

  vtkLog(INFO, << "Codec description");
  vtkLog(INFO, << "Name " << this->Codec->name);
  vtkLog(INFO, << "Long name " << this->Codec->long_name);
  vtkLog(INFO, << "Wrapper name " << this->Codec->wrapper_name);

  this->EncodeCtx = avcodec_alloc_context3(this->Codec);
  if (!this->EncodeCtx)
  {
    vtkLog(ERROR, << "Failed to allocate an encode context.");
    return false;
  }

  this->Packet = av_packet_alloc();
  if (!this->Packet)
  {
    vtkLog(ERROR, << "Failed to allocate an AVPacket");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkFFmpegEncoderInternals::Flush()
{
  vtkLogScopeFunction(TRACE);
  int ret = avcodec_send_frame(this->EncodeCtx, nullptr);

  /* Receive output in a loop until AVERROR_EOF */
  // Continue to get as many packets as possible from the encoder
  // until it gives AVERROR(EAGAIN) or AVERROR_EOF
  while (ret >= 0)
  {
    ret = avcodec_receive_packet(this->EncodeCtx, this->Packet);
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
  avcodec_flush_buffers(this->EncodeCtx);
}

//------------------------------------------------------------------------------
bool vtkFFmpegEncoderInternals::InitializeSWFrame()
{
  vtkLogScopeFunction(TRACE);

  // prepare an AVFrame. This will be the input to the encoder.
  this->SoftwareFrame = av_frame_alloc();
  if (!this->SoftwareFrame)
  {
    vtkLog(ERROR, << "Could not allocate software frame");
    return false;
  }
  this->SoftwareFrame->format = this->InputPixFmt;
  this->SoftwareFrame->width = this->EncodeCtx->width;
  this->SoftwareFrame->height = this->EncodeCtx->height;

  if (av_frame_get_buffer(this->SoftwareFrame, 0) < 0)
  {
    vtkLog(ERROR, << "Could not allocate software frame data.");
    return false;
  }
  this->SoftwareFrame->pts = 0;

  return true;
}

//------------------------------------------------------------------------------
bool vtkFFmpegEncoderInternals::InitializeHWFrame()
{
  this->HardwareFrame = av_frame_alloc();
  if (!this->HardwareFrame)
  {
    vtkLog(ERROR, << "Out of memory error. Could not allocate hardware frame.");
    return false;
  }

  if (av_hwframe_get_buffer(this->EncodeCtx->hw_frames_ctx, this->HardwareFrame, 0) < 0)
  {
    vtkLog(ERROR, << "Failed to allocate buffers for hardware frame");
    return false;
  }

  if (!this->HardwareFrame->hw_frames_ctx)
  {
    vtkLog(ERROR, << "Out of memory error. Failed to setup hardware frame properly.");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkFFmpegEncoderInternals::InitializeCodec()
{
  return avcodec_open2(this->EncodeCtx, this->Codec, nullptr) >= 0;
}

//------------------------------------------------------------------------------
bool vtkFFmpegEncoderInternals::SetupHWFrameCtx(AVPixelFormat HWPixelFormat)
{
  vtkLogScopeFunction(TRACE);
  AVBufferRef* hwFramesRef = nullptr;
  AVHWFramesContext* hwFramesCtx = nullptr;

  if (!(hwFramesRef = av_hwframe_ctx_alloc(this->HardwareDevCtx)))
  {
    vtkLog(ERROR, << "Failed to create hardware frame context.");
    return false;
  }

  hwFramesCtx = reinterpret_cast<AVHWFramesContext*>(hwFramesRef->data);
  hwFramesCtx->format = HWPixelFormat;
  hwFramesCtx->sw_format = this->InputPixFmt;
  hwFramesCtx->width = this->EncodeCtx->width;
  hwFramesCtx->height = this->EncodeCtx->height;
  hwFramesCtx->initial_pool_size = 1;
  if (av_hwframe_ctx_init(hwFramesRef) < 0)
  {
    vtkLog(ERROR, << "Failed to initialize hardware frame context.");
    av_buffer_unref(&hwFramesRef);
    return false;
  }

  this->EncodeCtx->hw_frames_ctx = av_buffer_ref(hwFramesRef);
  if (!this->EncodeCtx->hw_frames_ctx)
  {
    vtkLog(ERROR, << "Cannot allocate hardware frames. Out of memory");
    return false;
  }
  av_buffer_unref(&hwFramesRef);
  return true;
}

//------------------------------------------------------------------------------
bool vtkFFmpegEncoderInternals::PreprocessInput(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  bool success = true;
  const auto height = frame->GetHeight();
  const auto chromaHeight = frame->GetChromaHeight(height, frame->GetPixelFormat());
  auto tStart = std::chrono::high_resolution_clock::now();
  if (frame->GetPixelFormat() == VTKPixelFormatType::VTKPF_RGBA32)
  {
    success = this->ConvertRGBA32ToEncoderPixFmt(frame);
  }
  else if (frame->GetPixelFormat() == VTKPixelFormatType::VTKPF_IYUV)
  {
    if (av_frame_make_writable(this->SoftwareFrame) < 0)
    {
      vtkLog(ERROR, "Failed to make frame writable");
      return false;
    }
    int* strides = frame->GetStrides();
    for (int i = 0; i < 3; ++i)
    {
      this->SoftwareFrame->linesize[i] = strides[i];
    }
    auto array = frame->GetData();
    unsigned char* src = array->GetPointer(0);
    unsigned char* dst = this->SoftwareFrame->data[0];
    auto luma_end = src + strides[0] * frame->GetStorageHeight();
    std::copy(src, luma_end, dst);

    dst = this->SoftwareFrame->data[1];
    auto cb_end = luma_end + strides[0] * (chromaHeight >> 1);
    std::copy(luma_end, cb_end, dst);

    dst = this->SoftwareFrame->data[2];
    auto cr_end = cb_end + strides[0] * (chromaHeight >> 1);
    std::copy(cb_end, cr_end, dst);
  }
  else if (frame->GetPixelFormat() == VTKPixelFormatType::VTKPF_NV12)
  {
    if (av_frame_make_writable(this->SoftwareFrame) < 0)
    {
      vtkLog(ERROR, "Failed to make frame writable");
      return false;
    }
    int* strides = frame->GetStrides();
    this->SoftwareFrame->linesize[0] = strides[0];
    this->SoftwareFrame->linesize[1] = strides[1] << 1;

    auto array = frame->GetData();
    unsigned char* src = array->GetPointer(0);
    unsigned char* dst = this->SoftwareFrame->data[0];
    auto luma_end = src + strides[0] * frame->GetStorageHeight();
    std::copy(src, luma_end, dst);

    auto uv_end = luma_end + (strides[1] << 1) * chromaHeight;
    std::copy(luma_end, uv_end, &this->SoftwareFrame->data[1][0]);
  }
  this->dtScale = std::chrono::high_resolution_clock::now() - tStart;
  return success;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkFFmpegEncoderInternals::Encode(bool keyFrame /*=false*/)
{
  vtkLogScopeFunction(TRACE);
  VTKVideoEncoderResultType result;
  int statusCode = this->Send();
  auto status = ParseFFMPEGStatus(statusCode, /*during_send*/ true);
  if (statusCode < 0)
  {
    vtkLog(ERROR, << "Failed to send frame for encoding. Error - "
                  << vtkVideoProcessingStatusTypeUtilities::ToString(status) << " (" << statusCode
                  << ")");
    result.first = status;
    result.second = {};
    // don't return yet. try to receive.
  }

  statusCode = this->Receive();
  status = ParseFFMPEGStatus(statusCode, /*during_send*/ false);
  if (statusCode < 0)
  {
    vtkLog(ERROR, << "Failed to receive compressed video packet. Error - "
                  << vtkVideoProcessingStatusTypeUtilities::ToString(status) << " (" << statusCode
                  << ")");
    result.first = status;
    result.second = {};
  }
  else
  {
    result.first = status;
    result.second.push_back(vtk::TakeSmartPointer(this->PackageCompressedPacket()));
  }
  return result;
}

//------------------------------------------------------------------------------
int vtkFFmpegEncoderInternals::Send(bool keyFrame /*=false*/)
{
  vtkLogScopeFunction(TRACE);
  if (this->Frame)
  {
    vtkLog(TRACE, << "Send frame " << this->Frame->pts);
    this->Frame->pict_type = keyFrame ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_NONE;
  }
  else
  {
    vtkLog(TRACE, << "Drain the encoder");
  }

  this->t1 = std::chrono::high_resolution_clock::now();
  int ret = avcodec_send_frame(this->EncodeCtx, this->Frame);
  return ret;
}

//------------------------------------------------------------------------------
int vtkFFmpegEncoderInternals::Receive()
{
  vtkLogScopeFunction(TRACE);
  int result = avcodec_receive_packet(this->EncodeCtx, this->Packet);
  this->t2 = std::chrono::high_resolution_clock::now();
  this->dtEncode = this->t2 - this->t1;
  return result;
}

//------------------------------------------------------------------------------
vtkCompressedVideoPacket* vtkFFmpegEncoderInternals::PackageCompressedPacket()
{
  vtkLogScopeFunction(TRACE);
  auto result = vtkCompressedVideoPacket::New();
  result->SetSize(this->Packet->size);
  result->CopyData(this->Packet->data, this->Packet->size);
  result->SetIsKeyFrame(this->Packet->flags & AV_PICTURE_TYPE_I);
  result->SetPresentationTS(this->Packet->pts);
  result->SetWidth(this->Frame->width);
  result->SetHeight(this->Frame->height);
  vtkLog(TRACE, << "Pacakaged compressed frame " << this->Packet->size << " bytes");
  return result;
}

//------------------------------------------------------------------------------
void vtkFFmpegEncoderInternals::Tweak()
{
  vtkLogScopeFunction(TRACE);
  auto& codec = this->Codec;
  auto& ctx = this->EncodeCtx;
  if (codec->id == AV_CODEC_ID_VP9 && std::string(codec->name) == "libvpx-vp9")
  {
    // these settings are good for upto 1080p streams.
    // libvpx is simply incapable of encoding 4k realtime efficiently.
    // Source 1 :
    //  https://www.reddit.com/r/AV1/comments/k7colv/encoder_tuning_part_1_tuning_libvpxvp9_be_more/
    // Source 2 :
    //  https://developers.google.com/media/vp9/live-encoding
    // Source 3 :
    //  https://trac.ffMPEG.org/wiki/Encode/VP9
    av_opt_set(ctx->priv_data, "lag-in-frames", "0", 0);
    // default is good
    av_opt_set(ctx->priv_data, "quality", "realtime", 0);
    // default is good = 1000000 microseconds
    av_opt_set(ctx->priv_data, "deadline", "realtime", 0);
    // adjusts target cpu usage for realtime, -8: min cpu usage, +8 max cpu usage.
    av_opt_set(ctx->priv_data, "cpu-used", "8", 0);
    av_opt_set(ctx->priv_data, "frame-boost", "1", 0);
    av_opt_set(ctx->priv_data, "aq-mode", "0", 0);
    av_opt_set(ctx->priv_data, "tune-content", "screen", 0);
    av_opt_set(ctx->priv_data, "row-mt", "1", 0);
    av_opt_set(ctx->priv_data, "tile-columns", "4", 0);
    // force cbr.
    ctx->rc_min_rate = ctx->bit_rate;
    ctx->rc_max_rate = ctx->bit_rate;
    ctx->thread_count = 2;
  }
  else if (codec->id == AV_CODEC_ID_VP9 && this->EncodeCtx->pix_fmt == AV_PIX_FMT_VAAPI)
  {
    av_opt_set(ctx->priv_data, "rc_mode", "2", 0);        // cbr
    av_opt_set(ctx->priv_data, "idr_interval", "240", 0); // cbr
  }
  else if (codec->id == AV_CODEC_ID_AV1 && std::string(codec->name) == "libaom-av1")
  {
    av_opt_set(ctx->priv_data, "cpu-used", "4", 0); // default is 1!
    av_opt_set(ctx->priv_data, "lag-in-frames", "0", 0);
    av_opt_set(ctx->priv_data, "usage", "realtime", 0); // default is good.
    av_opt_set(ctx->priv_data, "row-mt", "1", 0);       // default auto.
    av_opt_set(ctx->priv_data, "tile-columns", "2", 0);
    av_opt_set(ctx->priv_data, "tile-rows", "1", 0);
    ctx->thread_count = 2;
  }
  else if (codec->id == AV_CODEC_ID_AV1 && std::string(codec->name) == "libsvtav1")
  {
    av_opt_set(ctx->priv_data, "svtav1-params", "rc=2:pred-struct=1:preset=12", 0);
  }
  else if (codec->id == AV_CODEC_ID_H264 && this->EncodeCtx->pix_fmt == AV_PIX_FMT_VAAPI)
  {
    av_opt_set(ctx->priv_data, "rc_mode", "2", 0); // cbr
  }
  else if (codec->id == AV_CODEC_ID_HEVC && this->EncodeCtx->pix_fmt == AV_PIX_FMT_VAAPI)
  {
    av_opt_set(ctx->priv_data, "rc_mode", "2", 0); // cbr
  }
  else if (codec->id == AV_CODEC_ID_H264 && this->EncodeCtx->pix_fmt == AV_PIX_FMT_CUDA)
  {
    av_opt_set(ctx->priv_data, "rc", "cbr", 0);
    av_opt_set(ctx->priv_data, "preset", "p4", 0);
    av_opt_set(ctx->priv_data, "tune", "ull", 0);
    av_opt_set(ctx->priv_data, "profile", "high", 0);
    av_opt_set(ctx->priv_data, "rc-lookahead", "0", 0);
    av_opt_set(ctx->priv_data, "multipass", "disabled", 0);
    av_opt_set(ctx->priv_data, "gpu", "any", 0);
    av_opt_set(ctx->priv_data, "delay", "0", 0);
    av_opt_set(ctx->priv_data, "forced-idr", "1", 0);
    av_opt_set(ctx->priv_data, "zerolatency", "1", 0);
  }
  else if (codec->id == AV_CODEC_ID_HEVC && this->EncodeCtx->pix_fmt == AV_PIX_FMT_CUDA)
  {
    // same as h264
    av_opt_set(ctx->priv_data, "rc", "cbr", 0);
    av_opt_set(ctx->priv_data, "preset", "p4", 0);
    av_opt_set(ctx->priv_data, "tune", "ull", 0);
    av_opt_set(ctx->priv_data, "profile", "main", 0);
    av_opt_set(ctx->priv_data, "rc-lookahead", "0", 0);
    av_opt_set(ctx->priv_data, "multipass", "disabled", 0);
    av_opt_set(ctx->priv_data, "gpu", "any", 0);
    av_opt_set(ctx->priv_data, "delay", "0", 0);
    av_opt_set(ctx->priv_data, "forced-idr", "1", 0);
    av_opt_set(ctx->priv_data, "zerolatency", "1", 0);
  }
  else if (codec->id == AV_CODEC_ID_H264 && std::string(codec->name) == "libx264")
  {
    // https://trac.ffMPEG.org/wiki/Encode/H.264#crf
    // Applies to libx265 as well.
    av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(ctx->priv_data, "rc-lookahead", "0", 0);
    av_opt_set(ctx->priv_data, "forced-idr", "1", 0);
  }
  else if (codec->id == AV_CODEC_ID_HEVC && std::string(codec->name) == "libx265")
  {
    // https://trac.ffMPEG.org/wiki/Encode/H.264#crf
    // Applies to libx265 as well.
    av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(ctx->priv_data, "forced-idr", "1", 0);
  }
  // TODO: Provide an interface in abstract video encoder for custom codec parameters.
  for (const auto& pair : this->CustomCodecParameters)
  {
    auto option = pair.first.c_str();
    auto value = pair.second.c_str();
    int ret = av_opt_set(ctx->priv_data, option, value, 0);
    switch (ret)
    {
      case AVERROR(ERANGE):
        vtkLogF(WARNING, "Value(%s) for the option (%s) is out of range.", value, option);
        break;
      case AVERROR(EINVAL):
        vtkLogF(WARNING, "Value(%s) for te option(%s) is invalid.", value, option);
        break;
      case AVERROR_OPTION_NOT_FOUND:
        vtkLogF(WARNING, "Option %s not found ", option);
        break;
      case 0:
      default:
        break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkFFmpegEncoderInternals::TearDownEncoderFrames()
{
  vtkLogScopeFunction(TRACE);
  if (this->SoftwareFrame != nullptr)
  {
    av_frame_free(&this->SoftwareFrame);
    this->SoftwareFrame = nullptr;
  }

  if (this->HardwareFrame != nullptr)
  {
    av_frame_free(&this->HardwareFrame);
    this->HardwareFrame = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkFFmpegEncoderInternals::Shutdown()
{
  vtkLogScopeFunction(TRACE);
  if (this->EncodeCtx != nullptr)
  {
    avcodec_free_context(&this->EncodeCtx);
    this->EncodeCtx = nullptr;
  }

  if (this->SoftwareFrame != nullptr)
  {
    av_frame_free(&this->SoftwareFrame);
    this->SoftwareFrame = nullptr;
  }

  if (this->HardwareFrame != nullptr)
  {
    av_frame_free(&this->HardwareFrame);
    this->HardwareFrame = nullptr;
  }

  if (this->Packet != nullptr)
  {
    av_packet_free(&this->Packet);
    this->Packet = nullptr;
  }

  if (this->HardwareDevCtx != nullptr)
  {
    av_buffer_unref(&this->HardwareDevCtx);
  }
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkFFmpegEncoderInternals::ParseFFMPEGStatus(
  int statusCode, bool during_send /*= true*/)
{
  VTKVideoProcessingStatusType result;
  switch (statusCode)
  {
    case 0:
      result = VTKVideoProcessingStatusType::VTKVPStatus_Success;
      break;
    case AVERROR_EOF:
      result = VTKVideoProcessingStatusType::VTKVPStatus_EOFError;
      break;
    case AVERROR(EINVAL):
      result = VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
      break;
    case AVERROR(ENOMEM):
      result = VTKVideoProcessingStatusType::VTKVPStatus_OutOfMemory;
      break;
    case AVERROR(EAGAIN):
      result = during_send ? VTKVideoProcessingStatusType::VTKVPStatus_TrySendAgain
                           : VTKVideoProcessingStatusType::VTKVPStatus_TryRecvAgain;
      break;
    case AVERROR_UNKNOWN:
    default:
      result = VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
      break;
  }
  return result;
}
