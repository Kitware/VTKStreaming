/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVpxDecoder.cxx

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVpxDecoder.h"
#include "vtkLogger.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRawVideoFrame.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include "vtkstreaming_libvpx.h"
#include VTKSTREAMINGLIBVPX_HEADER(vp8dx.h)
#include VTKSTREAMINGLIBVPX_HEADER(vpx_decoder.h)

#include <algorithm>
#include <cstdint>

struct vtkVpxDecoder::vtkInternals
{
  vpx_codec_ctx_t Ctx;
  vpx_codec_dec_cfg_t Cfg;
  vpx_codec_iface_t* (*const Interface)() = &vpx_codec_vp9_dx;
  vpx_codec_err_t Result;
};

vtkStandardNewMacro(vtkVpxDecoder);

//------------------------------------------------------------------------------
vtkVpxDecoder::vtkVpxDecoder()
  : Internals(new vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkVpxDecoder::~vtkVpxDecoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkVpxDecoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkIdType vtkVpxDecoder::GetLastDecodeTimeNS() const noexcept
{
  return 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkVpxDecoder::GetLastScaleTimeNS() const noexcept
{
  return 0;
}

//------------------------------------------------------------------------------
bool vtkVpxDecoder::SupportsCodec(VTKVideoCodecType codec) const noexcept
{
  return codec == VTKVideoCodecType::VTKVC_VP9;
}

//------------------------------------------------------------------------------
bool vtkVpxDecoder::InitializeInternal()
{
  auto& internals = (*this->Internals);
  internals.Cfg.threads = vtkMultiThreader::GetGlobalDefaultNumberOfThreads();
  internals.Cfg.w = this->Width;
  internals.Cfg.h = this->Height;
  if (vpx_codec_dec_init(&internals.Ctx, internals.Interface(), &internals.Cfg, 0))
  {
    vtkLogF(ERROR, "Failed to initialize vpx decoder.");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkVpxDecoder::ShutdownInternal()
{
  auto& internals = (*this->Internals);
  // destroy decoder context.
  internals.Result = vpx_codec_destroy(&internals.Ctx);
  if (internals.Result)
  {
    vtkLogF(ERROR, "Failed to destroy decoder context.");
  }
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkVpxDecoder::DecodeInternal(
  vtkSmartPointer<vtkCompressedVideoPacket> chunk)
{
  auto& internals = (*this->Internals);
  vpx_codec_iter_t iter = nullptr;
  vpx_image_t* img = nullptr;
  const uint8_t* data = chunk->GetData()->GetPointer(0);
  const unsigned int size = chunk->GetSize();
  internals.Result = vpx_codec_decode(&internals.Ctx, data, size, nullptr, 0);
  if (internals.Result != VPX_CODEC_OK)
  {
    vtkLogF(ERROR, "Failed to decode %s frame %lld. Error: %s",
      chunk->GetIsKeyFrame() ? "key" : "delta", chunk->GetPresentationTS(),
      vpx_codec_err_to_string(internals.Result));
    return { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} };
  }
  VTKVideoDecoderResultType result;
  result.first = VTKVideoProcessingStatusType::VTKVPStatus_Success;
  while ((img = vpx_codec_get_frame(&internals.Ctx, &iter)) != nullptr)
  {
    auto frame = vtk::TakeSmartPointer(vtkOpenGLVideoFrame::New());
    frame->SetWidth(img->d_w);
    frame->SetHeight(img->d_h);
    frame->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
    frame->SetContext(vtkOpenGLRenderWindow::SafeDownCast(this->GraphicsContext));
    frame->ComputeDefaultStrides();
    frame->AllocateDataStore();
    const int chromaHeight = img->h >> 1;
    bool success = true;
    switch (img->fmt)
    {
      case VPX_IMG_FMT_I420:
        frame->SetPixelFormat(VTKPixelFormatType::VTKPF_IYUV);
        frame->CopyPlanarData(img->planes[0], img->stride[0], img->h, 0);
        frame->CopyPlanarData(img->planes[1], img->stride[1], chromaHeight, 1);
        frame->CopyPlanarData(img->planes[2], img->stride[2], chromaHeight, 2);
        break;
      case VPX_IMG_FMT_NV12:
        frame->SetPixelFormat(VTKPixelFormatType::VTKPF_NV12);
        frame->CopyPlanarData(img->planes[0], img->stride[0], img->h, 0);
        frame->CopyPlanarData(img->planes[1], img->stride[1], chromaHeight, 1);
        break;
      default:
        vtkLogF(ERROR, "Unsupported vpx pixel format (%d)", int(img->fmt));
        success = false;
        break;
    }
    if (!success)
    {
      return { VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue, {} };
    }
    else
    {
      result.second.emplace_back(frame);
    }
  }
  return result;
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkVpxDecoder::SendEOS()
{
  return this->DecodeInternal(nullptr);
}
