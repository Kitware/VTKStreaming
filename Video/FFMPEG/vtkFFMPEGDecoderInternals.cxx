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
vtkRawVideoFrame* vtkFFMPEGDecoderInternals::GetOutputFrameFromDecodedFrame()
{
  vtkLogScopeFunction(TRACE);
  vtkLog(TRACE, << "Decoded frame dimensions " << this->SoftwareFrame->width << "x"
                << this->SoftwareFrame->height);
  vtkLog(TRACE, << "Decoded frame linsize " << this->SoftwareFrame->linesize[0] << "x"
                << this->SoftwareFrame->linesize[1] << 'x' << this->SoftwareFrame->linesize[2]);

  auto output = vtkRawVideoFrame::New();
  // Wrap the decoded frame into our vtkRawVideoFrame instance.
  output->SetWidth(this->SoftwareFrame->width);
  output->SetHeight(this->SoftwareFrame->height);
  output->SetIsKeyFrame(this->SoftwareFrame->key_frame);
  switch (this->SoftwareFrame->format)
  {
    case AV_PIX_FMT_RGB24:
      output->SetPixelFormat(VTKPixelFormat::RGB24);
      output->SetSliceOrder(vtkRawVideoFrame::SliceOrderType::BottomUp);
      break;
    case AV_PIX_FMT_RGBA:
      output->SetPixelFormat(VTKPixelFormat::RGBA32);
      output->SetSliceOrder(vtkRawVideoFrame::SliceOrderType::BottomUp);
      break;
    case AV_PIX_FMT_NV12:
      output->SetPixelFormat(VTKPixelFormat::NV12);
      output->SetSliceOrder(vtkRawVideoFrame::SliceOrderType::TopDown);
      break;
    case AV_PIX_FMT_YUV420P:
    default:
      output->SetPixelFormat(VTKPixelFormat::YUV420P);
      output->SetSliceOrder(vtkRawVideoFrame::SliceOrderType::TopDown);
      break;
  }
  // set the arrays with strides.
  // NOTE: upon receiving a next decoded frame, the data backing  output will be invalid.
  // Keep that in mind.
  output->SetStrides(this->SoftwareFrame->linesize, AV_NUM_DATA_POINTERS);
  for (int planeId = 0;
       planeId < VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES && planeId < AV_NUM_DATA_POINTERS; ++planeId)
  {
    int size = 0;
    if (planeId && this->SoftwareFrame->format != AV_PIX_FMT_RGBA &&
      this->SoftwareFrame->format != AV_PIX_FMT_RGB24)
    {
      size = this->SoftwareFrame->linesize[planeId] * (this->SoftwareFrame->height >> 1);
    }
    else
    {
      size = this->SoftwareFrame->linesize[planeId] * (this->SoftwareFrame->height);
    }
    output->SetSize(size, planeId);
    output->CopyData(this->SoftwareFrame->data[planeId], size, planeId);
  }

  return output;
}
