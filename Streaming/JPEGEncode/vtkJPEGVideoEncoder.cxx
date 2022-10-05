/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGVideoEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkJPEGVideoEncoder.h"

#include "vtkCompressedVideoPacket.h"
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPointData.h"
#include "vtkRawVideoFrame.h"
#include "vtkSmartPointer.h"
#include "vtkVideoCodecTypes.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <chrono>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkJPEGVideoEncoder);

//------------------------------------------------------------------------------
vtkJPEGVideoEncoder::vtkJPEGVideoEncoder()
  : Writer(vtkJPEGWriter::New())
  , GLFrame(vtkOpenGLVideoFrame::New())
{
  this->Writer->WriteToMemoryOn();
  this->InputPixelFormat = VTKPixelFormatType::VTKPF_RGBA32;
}

//------------------------------------------------------------------------------
vtkJPEGVideoEncoder::~vtkJPEGVideoEncoder()
{
  this->Shutdown();
  this->Writer->Delete();
  this->Writer = nullptr;
}

//------------------------------------------------------------------------------
void vtkJPEGVideoEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Writer->PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkJPEGVideoEncoder::InitializeInternal()
{
  return true;
}

//------------------------------------------------------------------------------
void vtkJPEGVideoEncoder::ShutdownInternal() {}

//------------------------------------------------------------------------------
void vtkJPEGVideoEncoder::FlushInternal() {}

//------------------------------------------------------------------------------
bool vtkJPEGVideoEncoder::SetupEncoderFrame(int w, int h)
{
  auto estSize =
    vtkRawVideoFrame::GetEstimatedSize(this->Width, this->Height, VTKPixelFormatType::VTKPF_RGBA32);
  if (estSize != this->GLFrame->GetActualSize())
  {
    this->GLFrame->ReleaseGraphicsResources();
    auto gfxContext = vtkOpenGLRenderWindow::SafeDownCast(this->GraphicsContext);
    this->GLFrame->SetContext(gfxContext);
    this->GLFrame->SetWidth(this->Width);
    this->GLFrame->SetHeight(this->Height);
    this->GLFrame->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
    this->GLFrame->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);
    this->GLFrame->ComputeDefaultStrides();
    this->GLFrame->AllocateDataStore();
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkJPEGVideoEncoder::TearDownEncoderFrame() {}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkJPEGVideoEncoder::DrainInternal()
{
  return { VTKVideoProcessingStatusType::VTKVPStatus_Success, {} };
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkJPEGVideoEncoder::PushInternal(VTKVideoEncoderInputType frame)
{
  vtkLogScopeFunction(TRACE);
  auto pixFmt = frame->GetPixelFormat();
  if (pixFmt != VTKPixelFormatType::VTKPF_RGBA32 && pixFmt != VTKPixelFormatType::VTKPF_RGB24)
  {
    vtkLogF(ERROR, "Invalid pixel format %s", vtkPixelFormatTypeUtilities::ToString(pixFmt));
    return VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
  }
  vtkNew<vtkImageData> img;
  img->SetDimensions(frame->GetStorageWidth(), frame->GetStorageHeight(), 1);
  if (pixFmt == VTKPixelFormatType::VTKPF_RGB24)
  {
    img->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  }
  else
  {
    img->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  }
  auto srcArr = frame->GetData();
  auto dst = reinterpret_cast<unsigned char*>(img->GetPointData()->GetScalars()->GetVoidPointer(0));
  auto src = srcArr->GetPointer(0);
  auto size = srcArr->GetNumberOfValues();
  std::copy(src, src + size, dst);
  this->Writer->SetInputData(img);
  this->Writer->SetQuality(this->Quality);
  return VTKVideoProcessingStatusType::VTKVPStatus_Success;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkJPEGVideoEncoder::GetResultInternal()
{
  vtkLogScopeFunction(TRACE);
  VTKVideoEncoderResultType result;
  auto tStart = std::chrono::high_resolution_clock::now();
  this->Writer->Write();
  this->EncodeTime = (std::chrono::high_resolution_clock::now() - tStart).count();

  result.first = VTKVideoProcessingStatusType::VTKVPStatus_Success;
  result.second.emplace_back(vtk::TakeSmartPointer(vtkCompressedVideoPacket::New()));

  auto packet = result.second.front();
  int dims[3] = {};
  this->Writer->GetInput()->GetDimensions(dims);
  packet->SetMimeType("image/jpeg");
  packet->SetWidth(dims[0]);
  packet->SetWidth(dims[1]);
  packet->SetSize(this->Writer->GetResult()->GetSize());
  packet->CopyData(this->Writer->GetResult());

  return result;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkJPEGVideoEncoder::EncodeInternal(VTKVideoEncoderInputType frame)
{
  vtkLogScopeFunction(TRACE);
  VTKVideoEncoderResultType result;
  result.first = this->PushInternal(frame);
  if (result.first != VTKVideoProcessingStatusType::VTKVPStatus_Success)
  {
    return result;
  }

  return this->GetResultInternal();
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkJPEGVideoEncoder::EncodeDisplayInternal()
{
  vtkLogScopeFunction(TRACE);

  this->GLFrame->Capture(this->GraphicsContext);

  return this->EncodeInternal(this->GLFrame);
}

//------------------------------------------------------------------------------
bool vtkJPEGVideoEncoder::SupportsCodec(VTKVideoCodecType codec) const noexcept
{
  return codec == VTKVideoCodecType::VTKVC_JPEG;
}

//------------------------------------------------------------------------------
vtkIdType vtkJPEGVideoEncoder::GetLastEncodeTimeNS() const noexcept
{
  vtkLogScopeFunction(TRACE);
  return this->EncodeTime;
}

//------------------------------------------------------------------------------
vtkIdType vtkJPEGVideoEncoder::GetLastScaleTimeNS() const noexcept
{
  return 0;
}
