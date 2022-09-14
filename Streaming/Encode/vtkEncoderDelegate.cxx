/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEncoderDelegate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkEncoderDelegate.h"
#include "vtkLogger.h"
#include <iostream>

vtkEncoderDelegate::vtkEncoderDelegate() = default;

vtkEncoderDelegate::~vtkEncoderDelegate() = default;

void vtkEncoderDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "BufferSize: " << this->BufferSize << "\n";
}

void vtkEncoderDelegate::InitializeWorker(VTKVideoEncodeWorkerType workerFunc)
{
  vtkLogScopeFunction(TRACE);
  return this->InitializeWorkerInternal(workerFunc);
}

void vtkEncoderDelegate::Flush()
{
  vtkLogScopeFunction(TRACE);
  return this->FlushInternal();
}

void vtkEncoderDelegate::Terminate()
{
  vtkLogScopeFunction(TRACE);
  return this->TerminateInternal();
}

void vtkEncoderDelegate::PushWorkUnit(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  return this->PushWorkUnitInternal(frame);
}

VTKVideoEncoderResultType vtkEncoderDelegate::GetResult()
{
  vtkLogScopeFunction(TRACE);
  return this->GetResultInternal();
}

void vtkEncoderDelegate::PrepareFrameInternal(
  vtkRawVideoFrame* frame, VTKVideoEncoderInputType& dstFrame)
{
  vtkLogScopeFunction(TRACE);
  dstFrame.TakeReference(frame->NewInstance());
  dstFrame->ShallowCopy(frame);
  dstFrame->AllocateDataStore();
  dstFrame->CopyFrameData(frame);
}
