/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVpxEncoder.cxx

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVpxEncoder.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

#include "vtkstreaming_libvpx.h"
#include "vtkstreaminglibvpx/vpx/vp8cx.h"
#include "vtkstreaminglibvpx/vpx/vpx_codec.h"

#include <algorithm>
#include <chrono>

struct vtkVpxEncoder::vtkInternals
{
  vpx_codec_ctx_t Ctx;
  vpx_codec_enc_cfg_t Cfg;
  vpx_codec_iface_t* (*const Interface)() = &vpx_codec_vp9_cx;
  vpx_image_t* RawImage = nullptr;
  vpx_codec_err_t Result;
  vpx_codec_pts_t SendCounter;
  // timing for encode and copy ops.
  std::chrono::high_resolution_clock::time_point::duration dtEncode, dtCopy;
};

namespace
{
// https://www.webmproject.org/vp9/levels/#example-frame-size-and-display-rate
// determine level from width, height and percieved encoder framerate.
unsigned int EstimateLevel(int width, int height, int fps_d, int fps_n)
{
  const int lumaSize = width * height;
  if (lumaSize <= 36864)
  {
    return 10;
  }
  else if (lumaSize <= 73728)
  {
    return 11;
  }
  else if (lumaSize <= 122880)
  {
    return 20;
  }
  else if (lumaSize <= 245760)
  {
    return 21;
  }
  else if (lumaSize <= 552960)
  {
    return 30;
  }
  else if (lumaSize <= 983040)
  {
    return 31;
  }
  else if ((lumaSize <= 2228224) && (fps_n <= 30))
  {
    return 40;
  }
  else if ((lumaSize <= 2228224) && (fps_n <= 60))
  {
    return 41;
  }
  else if ((lumaSize <= 8912896) && (fps_n <= 30))
  {
    return 50;
  }
  else if ((lumaSize <= 8912896) && (fps_n <= 60))
  {
    return 51;
  }
  else if ((lumaSize <= 8912896) && (fps_n <= 120))
  {
    return 52;
  }
  else if ((lumaSize <= 35651584) && (fps_n <= 30))
  {
    return 60;
  }
  else if ((lumaSize <= 35651584) && (fps_n <= 60))
  {
    return 61;
  }
  else if ((lumaSize <= 35651584) && (fps_n <= 120))
  {
    return 62;
  }
  return 0;
}

#define EncoderControl(ctx, ctrl_id, data)                                                         \
  do                                                                                               \
  {                                                                                                \
    auto result = vpx_codec_control(ctx, ctrl_id, data);                                           \
    if (result != vpx_codec_err_t::VPX_CODEC_OK)                                                   \
    {                                                                                              \
      vtkLogF(ERROR, "Failed to set control parameter %s=%d. Error : %s", #ctrl_id, data,          \
        vpx_codec_err_to_string(result));                                                          \
    }                                                                                              \
  } while (0)
}

vtkStandardNewMacro(vtkVpxEncoder);

//------------------------------------------------------------------------------
vtkVpxEncoder::vtkVpxEncoder()
  : Internals(new vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkVpxEncoder::~vtkVpxEncoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkVpxEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkIdType vtkVpxEncoder::GetLastEncodeTimeNS() const noexcept
{
  return this->Internals->dtEncode.count();
}

//------------------------------------------------------------------------------
vtkIdType vtkVpxEncoder::GetLastScaleTimeNS() const noexcept
{
  return this->Internals->dtCopy.count();
}

//------------------------------------------------------------------------------
bool vtkVpxEncoder::SupportsCodec(VTKVideoCodecType codec) const noexcept
{
  return codec == VTKVideoCodecType::VTKVC_VP9;
}

//------------------------------------------------------------------------------
bool vtkVpxEncoder::InitializeInternal()
{
  auto& internals = (*this->Internals);
  internals.SendCounter = 0;
  // an initial config.
  internals.Result = vpx_codec_enc_config_default(internals.Interface(), &internals.Cfg, 0);
  if (internals.Result)
  {
    vtkLogF(ERROR, "Failed to obtain default encoder config.");
    return false;
  }
  internals.Cfg.g_timebase.num = this->TimeBaseStart;
  internals.Cfg.g_timebase.den = this->TimeBaseEnd;
  internals.Cfg.g_error_resilient = this->ErrorResilient;
  internals.Cfg.g_lag_in_frames = 0;
  internals.Cfg.g_threads = 2;
  internals.Cfg.g_profile = 0;
  internals.Cfg.g_pass = VPX_RC_ONE_PASS;

  // fine tune keyframe interval
  internals.Cfg.kf_mode = vpx_kf_mode::VPX_KF_AUTO;
  internals.Cfg.kf_max_dist = this->GroupOfPicturesSize;

  // fine tune rate control settings
  internals.Cfg.rc_target_bitrate = this->BitRate / 1000;
  internals.Cfg.rc_max_quantizer = 63;
  internals.Cfg.rc_min_quantizer = vtkMath::ClampValue(
    this->QuantizationParameter, static_cast<unsigned int>(1), static_cast<unsigned int>(63));
  // internals.Cfg.rc_dropframe_thresh = 100;

  switch (this->BitRateControlMode)
  {
    case vtkVideoEncoder::BRCType::VBR:
      // vpx only supports vbr in two pass? two pass does not seem right for live encoding.
      internals.Cfg.rc_end_usage = vpx_rc_mode::VPX_CBR;
      vtkLog(WARNING, "Ignore VBR request. Enforce CBR mode.");
      break;
    case vtkVideoEncoder::BRCType::CQP:
      internals.Cfg.rc_end_usage = vpx_rc_mode::VPX_CQ;
      internals.Cfg.rc_min_quantizer = internals.Cfg.rc_max_quantizer;
      break;
    case vtkVideoEncoder::BRCType::CBR:
    default:
      internals.Cfg.rc_end_usage = vpx_rc_mode::VPX_CBR;
      break;
  }

  // disable temporal layering, use single spatial layer.
  std::fill(internals.Cfg.ss_enable_auto_alt_ref,
    internals.Cfg.ss_enable_auto_alt_ref + VPX_SS_MAX_LAYERS, 0);
  internals.Cfg.ss_number_layers = 1;
  internals.Cfg.temporal_layering_mode = VP9E_TEMPORAL_LAYERING_MODE_NOLAYERING;
  return true;
}

//------------------------------------------------------------------------------
void vtkVpxEncoder::ShutdownInternal()
{
  auto& internals = (*this->Internals);
  // free input resource.
  this->TearDownEncoderFrame();
  // destroy encoder context.
  internals.Result = vpx_codec_destroy(&internals.Ctx);
  if (internals.Result)
  {
    vtkLogF(ERROR, "Failed to destroy encoder context.");
  }
}

//------------------------------------------------------------------------------
bool vtkVpxEncoder::SetupEncoderFrame(int width, int height)
{
  vtkLogF(TRACE, "%s, %dx%d", __func__, width, height);
  auto& internals = (*this->Internals);
  // initialize encoder for these dimensions.
  internals.Cfg.g_w = this->Width;
  internals.Cfg.g_h = this->Height;
  internals.Result = vpx_codec_enc_init(&internals.Ctx, internals.Interface(), &internals.Cfg, 0);
  if (internals.Result)
  {
    vtkLogF(ERROR, "Failed to initialize encoder context. Error: %s, Detail: %s",
      vpx_codec_err_to_string(internals.Result), vpx_codec_error_detail(&internals.Ctx));
    return false;
  }

  // fine tune control parameters for live encoding.
  EncoderControl(&internals.Ctx, VP8E_SET_CPUUSED, this->TargetCPUUsage);         // -9..9
  EncoderControl(&internals.Ctx, VP8E_SET_ENABLEAUTOALTREF, 0);                   // 0 always.
  EncoderControl(&internals.Ctx, VP8E_SET_SHARPNESS, this->Sharpness);            // 0..7
  EncoderControl(&internals.Ctx, VP8E_SET_CQ_LEVEL, this->QuantizationParameter); // 0..63
  EncoderControl(
    &internals.Ctx, VP8E_SET_MAX_INTRA_BITRATE_PCT, int(this->MaxIntraBitrateFactor * 100));
  EncoderControl(
    &internals.Ctx, VP9E_SET_MAX_INTER_BITRATE_PCT, int(this->MaxInterBitrateFactor * 100));
  EncoderControl(&internals.Ctx, VP9E_SET_GF_CBR_BOOST_PCT, 0);
  EncoderControl(&internals.Ctx, VP9E_SET_LOSSLESS, 0);                     // false always
  EncoderControl(&internals.Ctx, VP9E_SET_TILE_COLUMNS, this->TileColumns); // default 6
  EncoderControl(&internals.Ctx, VP9E_SET_TILE_ROWS, this->TileRows);       // default 0
  EncoderControl(&internals.Ctx, VP9E_SET_FRAME_PARALLEL_DECODING, 0);      // default 1
  EncoderControl(&internals.Ctx, VP9E_SET_AQ_MODE, 0);                      // should be disabled
  EncoderControl(&internals.Ctx, VP9E_SET_TUNE_CONTENT,
    VP9E_CONTENT_SCREEN); // default is regular video content.
  EncoderControl(&internals.Ctx, VP9E_SET_COLOR_SPACE,
    vpx_color_space::VPX_CS_BT_709);                        // default should be BT_709
  EncoderControl(&internals.Ctx, VP9E_SET_COLOR_RANGE, 0);  // should be limited.
  EncoderControl(&internals.Ctx, VP9E_SET_TARGET_LEVEL, 0); // let encoder determine a level.
  EncoderControl(&internals.Ctx, VP9E_SET_ROW_MT, this->RowBasedMultiThreading); // false/true

  // create a new raw image object.
  vpx_img_fmt_t img_fmt;
  switch (this->InputPixelFormat)
  {
    case VTKPixelFormatType::VTKPF_RGBA32:
    case VTKPixelFormatType::VTKPF_RGB24:
      vtkLogF(ERROR, "Unsupported pixel format! VTKPF_RGBA32");
      internals.Result = vpx_codec_err_t::VPX_CODEC_INVALID_PARAM;
      break;
    case VTKPixelFormatType::VTKPF_NV12:
      img_fmt = VPX_IMG_FMT_NV12;
      break;
    case VTKPixelFormatType::VTKPF_IYUV:
      img_fmt = VPX_IMG_FMT_I420;
      break;
  }
  internals.RawImage = vpx_img_alloc(internals.RawImage, img_fmt, this->Width, this->Height, 8);
  return true;
}

//------------------------------------------------------------------------------
void vtkVpxEncoder::TearDownEncoderFrame()
{
  auto& internals = (*this->Internals);
  if (internals.RawImage)
  {
    // was allocated on the heap? let's free it.
    vpx_img_free(internals.RawImage);
    internals.RawImage = nullptr;
  }
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVpxEncoder::EncodeInternal(vtkSmartPointer<vtkRawVideoFrame> frame)
{
  auto& internals = (*this->Internals);
  // populate vpx_image with video frame data.
  auto img = internals.RawImage;
  if (frame != nullptr)
  {
    auto tc1 = std::chrono::high_resolution_clock::now();
    auto data = frame->GetData();
    const auto height = frame->GetHeight();
    const auto chromaHeight = frame->GetChromaHeight(height, frame->GetPixelFormat());
    if (frame->GetPixelFormat() == VTKPixelFormatType::VTKPF_IYUV)
    {
      int* strides = frame->GetStrides();
      for (int i = 0; i < 3; ++i)
      {
        img->stride[i] = strides[i];
      }
      unsigned char* src = data->GetPointer(0);
      unsigned char* dst = img->planes[0];
      auto luma_end = src + strides[0] * frame->GetStorageHeight();
      std::copy(src, luma_end, dst);

      dst = img->planes[1];
      auto cb_end = luma_end + strides[0] * (chromaHeight >> 1);
      std::copy(luma_end, cb_end, dst);

      dst = img->planes[2];
      auto cr_end = cb_end + strides[0] * (chromaHeight >> 1);
      std::copy(cb_end, cr_end, dst);
    }
    else if (frame->GetPixelFormat() == VTKPixelFormatType::VTKPF_NV12)
    {
      int* strides = frame->GetStrides();
      img->stride[0] = strides[0];
      img->stride[1] = strides[1] << 1;

      unsigned char* src = data->GetPointer(0);
      unsigned char* dst = img->planes[0];
      auto luma_end = src + strides[0] * frame->GetStorageHeight();
      std::copy(src, luma_end, dst);

      auto uv_end = luma_end + (strides[1] << 1) * chromaHeight;
      std::copy(luma_end, uv_end, &img->planes[1][0]);
    }

    auto tc2 = std::chrono::high_resolution_clock::now();
    internals.dtCopy = tc2 - tc1;
  }
  // encode frame.
  const int flags = (this->ForceIFrame || this->KeyFramesOnly) ? VPX_EFLAG_FORCE_KF : 0;
  // TODO: expose property.
  const int deadline = 1; //(this->Deadline);
  vpx_codec_iter_t iter = 0;
  const vpx_codec_cx_pkt_t* pkt = nullptr;
  auto te1 = std::chrono::high_resolution_clock::now();
  internals.Result =
    vpx_codec_encode(&internals.Ctx, img, internals.SendCounter++, 1, flags, deadline);
  if (internals.Result != VPX_CODEC_OK)
  {
    vtkLogF(ERROR, "Failed to encode %s frame %ld. Error: %s, Detail: %s", flags ? "key" : "delta",
      internals.SendCounter, vpx_codec_err_to_string(internals.Result),
      vpx_codec_error_detail(&internals.Ctx));
    return { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} };
  }
  VTKVideoEncoderResultType result;
  result.first = VTKVideoProcessingStatusType::VTKVPStatus_Success;
  while ((pkt = vpx_codec_get_cx_data(&internals.Ctx, &iter)) != nullptr)
  {
    auto chunk = vtk::TakeSmartPointer(vtkCompressedVideoPacket::New());
    if (pkt->kind == VPX_CODEC_CX_FRAME_PKT)
    {
      chunk->SetIsKeyFrame((pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0);
      // TODO: actually build the parameter string instead of hard-coding it.
      chunk->SetCodecLongName("vp09.00.10.08");
      chunk->SetWidth(pkt->data.frame.width[0]);
      chunk->SetHeight(pkt->data.frame.height[0]);
      chunk->SetMimeType("application/octet-stream");
      chunk->SetPresentationTS(pkt->data.frame.pts);
      chunk->CopyData(reinterpret_cast<unsigned char*>(pkt->data.frame.buf), pkt->data.frame.sz);
      vtkLogF(TRACE, "%lld|%s|%d bytes", chunk->GetPresentationTS(),
        chunk->GetIsKeyFrame() ? "key" : "delta", chunk->GetSize());
      result.second.emplace_back(chunk);
    }
  }
  auto te2 = std::chrono::high_resolution_clock::now();
  internals.dtEncode = te2 - te1;
  return result;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVpxEncoder::SendEOS()
{
  return this->EncodeInternal(nullptr);
}
