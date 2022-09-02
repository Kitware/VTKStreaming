/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGEncoderInternals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFMPEGEncoderInternals.h"
#include "vtkCodedVideoPacket.h"
#include "vtkLogger.h"
#include "vtkRawVideoFrame.h"

#include <cstdint>

extern "C"
{
#include <libavutil/pixfmt.h>
}

//------------------------------------------------------------------------------
bool vtkFFMPEGEncoderInternals::ConvertRGBA32ToEncoderPixFmt(vtkRawVideoFrame* rgba32Image)
{
  vtkLogScopeFunction(TRACE);

  const int& srcW = rgba32Image->GetWidth();
  const int& srcH = rgba32Image->GetHeight();
  const int& dstW = this->SoftwareFrame->width;
  const int& dstH = this->SoftwareFrame->height;

  vtkLog(TRACE, "Scale " << srcW << 'x' << srcH << "->" << dstW << 'x' << dstH);

  unsigned char* rgba32UcharArr = nullptr;
  auto size = rgba32Image->GetData(rgba32UcharArr);

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
  // ffmpeg requires top-down ordering. if slice order is bottomup, convert it.
  int sign = 1;
  if (rgba32Image->GetSliceOrder() == vtkRawVideoFrame::SliceOrderType::BottomUp)
  {
    sign = -1;
    rgba32UcharArr += static_cast<ptrdiff_t>(4 * srcW * (srcH - 1));
  }
  // sws_scale requires a full const cast.
  auto rgba32 = (const uint8_t* const*)&rgba32UcharArr;
  // r, g, b, a -> four components, total size = 4 * w * h, linesize = total_size/h
  const int inLinesize[1] = { sign * 4 * srcW };
  sws_scale(this->SwScaleCtx, rgba32, inLinesize, 0, srcH, this->SoftwareFrame->data,
    this->SoftwareFrame->linesize);
  return true;
}

//------------------------------------------------------------------------------
bool vtkFFMPEGEncoderInternals::PrepareForEncoding()
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
bool vtkFFMPEGEncoderInternals::InitializeHWEncodeCtx(AVHWDeviceType type)
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
bool vtkFFMPEGEncoderInternals::InitializeBasicEncodeCtx()
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
void vtkFFMPEGEncoderInternals::Flush()
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
bool vtkFFMPEGEncoderInternals::InitializeSWFrame()
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
  this->LastEncodedFrameDims[0] = this->EncodeCtx->width;
  this->LastEncodedFrameDims[1] = this->EncodeCtx->width;
  this->SoftwareFrame->pts = 0;

  return true;
}

//------------------------------------------------------------------------------
bool vtkFFMPEGEncoderInternals::InitializeHWFrame()
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
bool vtkFFMPEGEncoderInternals::InitializeCodec()
{
  return avcodec_open2(this->EncodeCtx, this->Codec, nullptr) >= 0;
}

//------------------------------------------------------------------------------
bool vtkFFMPEGEncoderInternals::SetupHWFrameCtx(AVPixelFormat HWPixelFormat)
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
bool vtkFFMPEGEncoderInternals::PreprocessInput(vtkRawVideoFrame* rgba32Image)
{
  vtkLogScopeFunction(TRACE);
  return this->ConvertRGBA32ToEncoderPixFmt(rgba32Image);
}

//------------------------------------------------------------------------------
int vtkFFMPEGEncoderInternals::Send(bool keyFrame /*=false*/)
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

  int ret = avcodec_send_frame(this->EncodeCtx, this->Frame);
  return ret;
}

//------------------------------------------------------------------------------
// TODO: Think of a way to not let this function block the calling thread.
bool vtkFFMPEGEncoderInternals::Encode(bool isKeyFrame, PacketRecvCallbackT& packetReceiver)
{
  vtkLogScopeFunction(TRACE);
  bool ready = this->PrepareForEncoding();
  if (!ready)
  {
    return false;
  }

  auto tStart = std::chrono::high_resolution_clock::now();
  int status = this->Send(isKeyFrame);
  if (status < 0)
  {
    this->dtEncode = std::chrono::nanoseconds(0);
    return false;
  }

  /* Receive output in a loop. */
  // Continue to get as many packets as possible from the encoder
  // until it gives AVERROR(EAGAIN) or AVERROR_EOF
  while (status >= 0)
  {
    status = this->Receive();
    auto now = std::chrono::high_resolution_clock::now();
    this->dtEncode = now - tStart;
    tStart = now;

    switch (status)
    {
      case AVERROR(EAGAIN):
      {
        vtkLog(
          TRACE, << "Encoder needs more input frames. Current frame->pts=" << this->Frame->pts);
        return true;
      }
      case AVERROR_EOF:
      {
        vtkLog(TRACE, << "Encoder input is flushed. No more output packets.");
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

    if (status < 0)
    {
      vtkLog(ERROR, "Error during encoding");
      return false;
    }
    else
    {
      vtkNew<vtkCodedVideoPacket> pkt;
      pkt->SetArray(this->Packet->data, this->Packet->size);
      pkt->SetIsKeyFrame(this->Packet->flags & AV_PICTURE_TYPE_I);
      pkt->SetPresentationTS(this->Packet->pts);
      pkt->SetWidth(this->Frame->width);
      pkt->SetHeight(this->Frame->height);
      vtkLog(TRACE, << "Successfully encoded, pktSize " << this->Packet->size << "bytes");
      packetReceiver(pkt.GetPointer());
    }
    ++this->FrameCounter;
  }
  return true;
}

//------------------------------------------------------------------------------
int vtkFFMPEGEncoderInternals::Receive()
{
  vtkLogScopeFunction(TRACE);
  return avcodec_receive_packet(this->EncodeCtx, this->Packet);
}

//------------------------------------------------------------------------------
void vtkFFMPEGEncoderInternals::Tweak()
{
  vtkLogScopeFunction(TRACE);
  auto& codec = this->Codec;
  auto& ctx = this->EncodeCtx;
  if (codec->id == AV_CODEC_ID_VP9 && std::string(codec->name) == "libvpx-vp9")
  {
    // https://www.reddit.com/r/AV1/comments/k7colv/encoder_tuning_part_1_tuning_libvpxvp9_be_more/
    av_opt_set(ctx->priv_data, "lag-in-frames", "0", 0);
    av_opt_set(ctx->priv_data, "rc_lookahead", "0", 0);
    av_opt_set(ctx->priv_data, "quality", "realtime", 0);
    av_opt_set(ctx->priv_data, "speed", "8", 0); // 5: high-quality, 8: low-quality
    av_opt_set(ctx->priv_data, "frame-boost", "1", 0);
    av_opt_set(ctx->priv_data, "tune-content", "screen", 0); // for live-streaming the renders
    av_opt_set(ctx->priv_data, "deadline", "realtime", 0);   // default is good
    av_opt_set(ctx->priv_data, "row-mt", "1", 0);            // default disabled.
    av_opt_set(ctx->priv_data, "speed", "16", 0);            // default is 1
  }
  else if (codec->id == AV_CODEC_ID_VP9 && this->EncodeCtx->pix_fmt == AV_PIX_FMT_VAAPI)
  {
    av_opt_set(ctx->priv_data, "rc_mode", "2", 0); // cbr
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
    // https://trac.ffmpeg.org/wiki/Encode/H.264#crf
    // Applies to libx265 as well.
    av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(ctx->priv_data, "rc-lookahead", "0", 0);
    av_opt_set(ctx->priv_data, "forced-idr", "1", 0);
  }
  else if (codec->id == AV_CODEC_ID_HEVC && std::string(codec->name) == "libx265")
  {
    // https://trac.ffmpeg.org/wiki/Encode/H.264#crf
    // Applies to libx265 as well.
    av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(ctx->priv_data, "forced-idr", "1", 0);
  }
  else if (std::string(codec->name) == "libaom-av1")
  {
    av_opt_set(ctx->priv_data, "cpu-used", "3", 0); // default is 1!
    av_opt_set(ctx->priv_data, "lag-in-frames", "0", 0);
    av_opt_set(ctx->priv_data, "usage", "realtime", 0); // default is good.
    av_opt_set(ctx->priv_data, "row-mt", "1", 0);       // default auto.
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
void vtkFFMPEGEncoderInternals::TearDownEncoderFrames()
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
void vtkFFMPEGEncoderInternals::Shutdown()
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
