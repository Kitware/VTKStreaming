/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMockVideoDecoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMockVideoDecoder.h"
#include "vtkCompressedVideoPacket.h"
#include "vtkObjectFactory.h"
#include "vtkVideoProcessingStatusTypes.h"

vtkStandardNewMacro(vtkMockVideoDecoder);

//------------------------------------------------------------------------------
vtkMockVideoDecoder::vtkMockVideoDecoder() = default;

//------------------------------------------------------------------------------
vtkMockVideoDecoder::~vtkMockVideoDecoder() = default;

//------------------------------------------------------------------------------
void vtkMockVideoDecoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkIdType vtkMockVideoDecoder::GetLastDecodeTimeNS() const noexcept
{
  return 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkMockVideoDecoder::GetLastScaleTimeNS() const noexcept
{
  return 0;
}

//------------------------------------------------------------------------------
bool vtkMockVideoDecoder::InitializeInternal()
{
  return true;
}

//------------------------------------------------------------------------------
void vtkMockVideoDecoder::ShutdownInternal() {}

//------------------------------------------------------------------------------
void vtkMockVideoDecoder::FlushInternal() {}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkMockVideoDecoder::PushInternal(vtkCompressedVideoPacket* packet)
{
  return VTKVideoProcessingStatusType::VTKVPStatus_Success;
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkMockVideoDecoder::GetResultInternal()
{
  return VTKVideoDecoderResultType({ VTKVideoProcessingStatusType::VTKVPStatus_Success, {} });
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkMockVideoDecoder::DecodeInternal(vtkCompressedVideoPacket* packet)
{
  return VTKVideoDecoderResultType({ VTKVideoProcessingStatusType::VTKVPStatus_Success, {} });
}

//------------------------------------------------------------------------------
void vtkMockVideoDecoder::DrainInternal() {}
