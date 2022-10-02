/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGVideoDecoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkJPEGVideoDecoder.h"

#include "vtkCompressedVideoPacket.h"
#include "vtkImageData.h"
#include "vtkJPEGReader.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkVideoCodecTypes.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <chrono>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkJPEGVideoDecoder);

//------------------------------------------------------------------------------
vtkJPEGVideoDecoder::vtkJPEGVideoDecoder()
  : Reader(vtkJPEGReader::New())
{
}

//------------------------------------------------------------------------------
vtkJPEGVideoDecoder::~vtkJPEGVideoDecoder()
{
  this->Shutdown();
  this->Reader->Delete();
  this->Reader = nullptr;
}

//------------------------------------------------------------------------------
void vtkJPEGVideoDecoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Reader->PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkJPEGVideoDecoder::InitializeInternal()
{
  vtkLogScopeFunction(TRACE);
  this->Reader->SetDataScalarTypeToUnsignedChar();
  this->Reader->SetDataByteOrderToLittleEndian();
  return true;
}

//------------------------------------------------------------------------------
void vtkJPEGVideoDecoder::ShutdownInternal() {}

//------------------------------------------------------------------------------
void vtkJPEGVideoDecoder::FlushInternal() {}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkJPEGVideoDecoder::DrainInternal()
{
  return { VTKVideoProcessingStatusType::VTKVPStatus_Success, {} };
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkJPEGVideoDecoder::PushInternal(vtkCompressedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  this->Reader->SetMemoryBufferLength(packet->GetSize());
  this->Reader->SetMemoryBuffer(packet->GetData()->GetPointer(0));

  auto tStart = std::chrono::high_resolution_clock::now();
  this->Reader->Update();
  this->DecodeTime = (std::chrono::high_resolution_clock::now() - tStart).count();

  return VTKVideoProcessingStatusType::VTKVPStatus_Success;
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkJPEGVideoDecoder::GetResultInternal()
{
  vtkLogScopeFunction(TRACE);
  VTKVideoDecoderResultType result;

  result.first = VTKVideoProcessingStatusType::VTKVPStatus_Success;
  result.second.emplace_back(vtk::TakeSmartPointer(vtkOpenGLVideoFrame::New()));

  auto frame = result.second.front();
  int dims[3] = {};
  auto img = vtk::MakeSmartPointer(this->Reader->GetOutput());
  img->GetDimensions(dims);
  frame->SetWidth(dims[0]);
  frame->SetHeight(dims[1]);
  frame->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32); // guess.
  frame->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);
  frame->ComputeDefaultStrides();
  frame->AllocateDataStore();

  auto src = reinterpret_cast<unsigned char*>(img->GetScalarPointer());
  frame->CopyData(src, dims[0] * 4, dims[1]);

  return result;
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkJPEGVideoDecoder::DecodeInternal(vtkCompressedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  VTKVideoDecoderResultType result;
  result.first = this->PushInternal(packet);
  if (result.first != VTKVideoProcessingStatusType::VTKVPStatus_Success)
  {
    return result;
  }

  return this->GetResultInternal();
}

//------------------------------------------------------------------------------
bool vtkJPEGVideoDecoder::SupportsCodec(VTKVideoCodecType codec) const noexcept
{
  return codec == VTKVideoCodecType::VTKVC_JPEG;
}

//------------------------------------------------------------------------------
vtkIdType vtkJPEGVideoDecoder::GetLastDecodeTimeNS() const noexcept
{
  vtkLogScopeFunction(TRACE);
  return this->DecodeTime;
}

//------------------------------------------------------------------------------
vtkIdType vtkJPEGVideoDecoder::GetLastScaleTimeNS() const noexcept
{
  return 0;
}
