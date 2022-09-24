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
#include "vtkCPUVideoFrame.h"
#include "vtkLogger.h"
#include "vtkRawVideoFrame.h"

#include <cstddef>
#include <cstdint>

extern "C"
{
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

//------------------------------------------------------------------------------
vtkCPUVideoFrame* vtkFFmpegDecoderInternals::GetOutputFrameFromDecodedFrame()
{
  vtkLogScopeFunction(TRACE);
  vtkLog(TRACE, << "Decoded frame dimensions " << this->SoftwareFrame->width << "x"
                << this->SoftwareFrame->height);
  vtkLog(TRACE, << "Decoded frame linsize " << this->SoftwareFrame->linesize[0] << "x"
                << this->SoftwareFrame->linesize[1] << 'x' << this->SoftwareFrame->linesize[2]);

  auto output = vtkCPUVideoFrame::New();
  // Wrap the decoded frame into our vtkCPUVideoFrame instance.
  output->SetWidth(this->SoftwareFrame->width);
  output->SetHeight(this->SoftwareFrame->height);
  output->SetIsKeyFrame(this->SoftwareFrame->key_frame);
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
    case AV_PIX_FMT_YUV420P:
    default:
      output->SetPixelFormat(VTKPixelFormatType::VTKPF_IYUV);
      output->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
      break;
  }
  output->SetWidth(this->SoftwareFrame->width);
  output->SetHeight(this->SoftwareFrame->height);
  output->SetStrides(this->SoftwareFrame->linesize, AV_NUM_DATA_POINTERS);
  output->AllocateDataStore();
  unsigned char* dstPixels = nullptr;
  int lastSize = 0;
  const auto dstSize = output->GetData(dstPixels);
  vtkLogF(TRACE, "DstSize=%d", dstSize);
  std::fill(dstPixels, dstPixels + dstSize, 0);

  for (int planeId = 0; planeId < 3; ++planeId)
  {
    uint8_t* srcPixels = this->SoftwareFrame->data[planeId];
    int srcLinesize = this->SoftwareFrame->linesize[planeId];
    std::ptrdiff_t numRows = 0;
    if (planeId && this->SoftwareFrame->format != AV_PIX_FMT_RGBA &&
      this->SoftwareFrame->format != AV_PIX_FMT_RGB24)
    {
      numRows = this->SoftwareFrame->height >> 1;
    }
    else
    {
      numRows = this->SoftwareFrame->height;
    }
    std::copy(srcPixels, srcPixels + numRows * srcLinesize, &dstPixels[lastSize]);
    vtkLogF(TRACE, "PlaneID=%d, size=%ld", planeId, numRows * srcLinesize);
    lastSize += numRows * srcLinesize;
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
