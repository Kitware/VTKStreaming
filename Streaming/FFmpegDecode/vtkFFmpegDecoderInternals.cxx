/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFmpegDecoderInternals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFmpegDecoderInternals.h"
#include "vtkLogger.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkRawVideoFrame.h"

#include <cstddef>
#include <cstdint>

extern "C"
{
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

//------------------------------------------------------------------------------
vtkOpenGLVideoFrame* vtkFFmpegDecoderInternals::GetOutputFrameFromDecodedFrame(
  vtkRenderWindow* context)
{
  vtkLogScopeFunction(TRACE);
  vtkLog(TRACE, << "Decoded frame dimensions " << this->SoftwareFrame->width << "x"
                << this->SoftwareFrame->height);
  vtkLog(TRACE, << "Decoded frame linsize " << this->SoftwareFrame->linesize[0] << "x"
                << this->SoftwareFrame->linesize[1] << 'x' << this->SoftwareFrame->linesize[2]);

  auto output = vtkOpenGLVideoFrame::New();
  // Wrap the decoded frame into our vtkOpenGLVideoFrame instance.
  switch (this->SoftwareFrame->format)
  {
    case AV_PIX_FMT_RGB24:
      output->SetPixelFormat(VTKPixelFormatType::VTKPF_RGB24);
      output->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);
      break;
    case AV_PIX_FMT_RGBA:
      output->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
      output->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);
      break;
    case AV_PIX_FMT_NV12:
      output->SetPixelFormat(VTKPixelFormatType::VTKPF_NV12);
      output->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
    case AV_PIX_FMT_YUV420P:
    default:
      output->SetPixelFormat(VTKPixelFormatType::VTKPF_IYUV);
      output->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
      break;
  }
  output->SetWidth(this->SoftwareFrame->width);
  output->SetHeight(this->SoftwareFrame->height);
  output->SetIsKeyFrame(this->SoftwareFrame->key_frame);
  output->SetContext(vtkOpenGLRenderWindow::SafeDownCast(context));
  output->AllocateDataStore();

  int nshifts[3] = { 0, 1, 1 };
  for (int i = 0; i < 3; ++i)
  {
    uint8_t* src = this->SoftwareFrame->data[i];
    int rowsize = this->SoftwareFrame->linesize[i];
    int numrows = this->SoftwareFrame->height >> nshifts[i];
    output->CopyPlanarData(src, rowsize, numrows, i);
  }

  return output;
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkFFmpegDecoderInternals::ParseFFMPEGStatus(
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
