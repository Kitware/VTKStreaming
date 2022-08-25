/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGDecoderInternals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFMPEGDecoderInternals.h"
#include "vtkLogger.h"
#include "vtkRawVideoFrame.h"

extern "C"
{
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

//------------------------------------------------------------------------------
bool vtkFFMPEGDecoderInternals::IsOutputFrameOutdated()
{
  vtkLogScopeFunction(TRACE);
  return this->OutputAVFrame != nullptr &&
    (this->OutputAVFrame->width != this->SoftwareFrame->width ||
      this->OutputAVFrame->height != this->SoftwareFrame->height);
}

//------------------------------------------------------------------------------
bool vtkFFMPEGDecoderInternals::InitializeOutputFrame(AVPixelFormat pixFmt)
{
  vtkLogScopeFunction(TRACE);
  if (this->IsOutputFrameOutdated())
  {
    // buffers are outdated, need to allocate new frame data.
    av_frame_free(&this->OutputAVFrame);
    this->OutputAVFrame = nullptr;
  }
  else if (this->OutputAVFrame != nullptr)
  {
    return true;
  }

  vtkLog(TRACE, "Setting up new output frame");
  this->OutputAVFrame = av_frame_alloc();
  if (!this->OutputAVFrame)
  {
    vtkLog(ERROR, "Failed to allocate output frame");
    return false;
  }

  this->OutputAVFrame->width = this->SoftwareFrame->width;
  this->OutputAVFrame->height = this->SoftwareFrame->height;
  this->OutputAVFrame->format = pixFmt;

  if (av_frame_get_buffer(this->OutputAVFrame, 0) < 0)
  {
    vtkLog(ERROR, << "Failed to allocate output frame data");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkFFMPEGDecoderInternals::GetOutputFrameFromDecodedFrame()
{
  vtkLogScopeFunction(TRACE);
  vtkLog(TRACE, << "Decoded frame dimensions " << this->SoftwareFrame->width << "x"
                << this->SoftwareFrame->height);
  vtkLog(TRACE, << "Decoded frame linsize " << this->SoftwareFrame->linesize[0] << "x"
                << this->SoftwareFrame->linesize[1] << 'x' << this->SoftwareFrame->linesize[2]);
  vtkLog(TRACE, << "Output frame dimensions " << this->OutputAVFrame->width << "x"
                << this->OutputAVFrame->height);

  auto tStart = std::chrono::high_resolution_clock::now();

  this->SwScaleCtx = sws_getCachedContext(this->SwScaleCtx, this->SoftwareFrame->width,
    this->SoftwareFrame->height, AVPixelFormat(this->SoftwareFrame->format),
    this->OutputAVFrame->width, this->OutputAVFrame->height,
    AVPixelFormat(this->OutputAVFrame->format), 0, nullptr, nullptr, nullptr);

  if (!this->SwScaleCtx)
  {
    vtkLog(ERROR, << "Could not initialize a scaling context for given parameters");
    return false;
  }

  // SoftwareFrame is always top-down, so we need to flip the data when output requires bottom-up.
  if (this->OutputVideoFrame->GetSliceOrder() == vtkRawVideoFrame::SliceOrderType::BottomUp)
  {
    for (int i = 0; i < 4; i++)
    {
      if (i &&
        (this->SoftwareFrame->format != AV_PIX_FMT_RGB24 &&
          this->SoftwareFrame->format != AV_PIX_FMT_RGBA))
      {
        this->SoftwareFrame->data[i] += static_cast<ptrdiff_t>(
          this->SoftwareFrame->linesize[i] * ((this->SoftwareFrame->height >> 1) - 1));
        this->SoftwareFrame->linesize[i] = -this->SoftwareFrame->linesize[i];
      }
      else
      {
        this->SoftwareFrame->data[i] += static_cast<ptrdiff_t>(
          this->SoftwareFrame->linesize[i] * (this->SoftwareFrame->height - 1));
        this->SoftwareFrame->linesize[i] = -this->SoftwareFrame->linesize[i];
      }
    }
  }

  sws_scale(this->SwScaleCtx, (const uint8_t* const*)this->SoftwareFrame->data,
    this->SoftwareFrame->linesize, 0, this->SoftwareFrame->height, this->OutputAVFrame->data,
    this->OutputAVFrame->linesize);

  this->dtScale = std::chrono::high_resolution_clock::now() - tStart;

  // Prepare the callback payload.
  this->OutputVideoFrame->SetWidth(this->OutputAVFrame->width);
  this->OutputVideoFrame->SetHeight(this->OutputAVFrame->height);
  this->OutputVideoFrame->SetIsKeyFrame(this->OutputAVFrame->key_frame);
  this->OutputVideoFrame->SetPixelFormat(
    this->OutputPixFmt == AV_PIX_FMT_RGBA ? VTKPixelFormat::RGBA32 : VTKPixelFormat::YUV420P);
  this->OutputVideoFrame->SetArray(this->OutputAVFrame->data[0], this->OutputAVFrame->linesize[0]);

  return true;
}
