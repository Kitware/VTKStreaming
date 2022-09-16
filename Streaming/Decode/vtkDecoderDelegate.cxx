/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDecoderDelegate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDecoderDelegate.h"
#include "vtkLogger.h"

vtkDecoderDelegate::vtkDecoderDelegate() = default;

vtkDecoderDelegate::~vtkDecoderDelegate() = default;

void vtkDecoderDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

void vtkDecoderDelegate::InitializeWorker(VTKVideoDecodeWorkerType workerFunc)
{
  vtkLogScopeFunction(TRACE);
  return this->InitializeWorkerInternal(workerFunc);
}

void vtkDecoderDelegate::Flush()
{
  vtkLogScopeFunction(TRACE);
  return this->FlushInternal();
}

void vtkDecoderDelegate::Terminate()
{
  vtkLogScopeFunction(TRACE);
  return this->TerminateInternal();
}

void vtkDecoderDelegate::PushWorkUnit(vtkCompressedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  return this->PushWorkUnitInternal(packet);
}

VTKVideoDecoderResultType vtkDecoderDelegate::GetResult()
{
  vtkLogScopeFunction(TRACE);
  return this->GetResultInternal();
}

void vtkDecoderDelegate::PreparePacketInternal(
  vtkCompressedVideoPacket* packet, VTKVideoDecoderInputType& dstPacket)
{
  vtkLogScopeFunction(TRACE);
  dstPacket.TakeReference(vtkCompressedVideoPacket::New());
  dstPacket->CopyMetadata(packet);
  dstPacket->AllocateForCopy(packet);

  // copy data from all planes.
  unsigned char* srcData = nullptr;
  unsigned char* dstData = nullptr;
  const int srcSize = packet->GetData(srcData);
  if (srcSize > 0)
  {
    const int dstSize = dstPacket->GetData(dstData);
    assert(dstSize == srcSize);
    dstPacket->CopyData(srcData, srcSize);
  }
}
