/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronousDecoderDelegate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSynchronousEncoderDelegate.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSynchronousEncoderDelegate);

vtkSynchronousEncoderDelegate::vtkSynchronousEncoderDelegate() = default;

vtkSynchronousEncoderDelegate::~vtkSynchronousEncoderDelegate() = default;

void vtkSynchronousEncoderDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "No. of inputs : " << this->Frames.size() << '\n';
  os << "No. of results : " << this->Results.size() << '\n';
}

void vtkSynchronousEncoderDelegate::ResetQueues()
{
  vtkLogScopeFunction(TRACE);
  auto frameQueue = std::queue<VTKVideoEncoderInputType>();
  this->Frames.swap(frameQueue);

  auto resultsQueue = std::queue<VTKVideoEncoderResultType>();
  this->Results.swap(resultsQueue);
}

void vtkSynchronousEncoderDelegate::InitializeWorkerInternal(VTKVideoEncodeWorkerType workerFunc)
{
  vtkLogScopeFunction(TRACE);
  this->Worker = workerFunc;
  this->ResetQueues();
}

void vtkSynchronousEncoderDelegate::FlushInternal()
{
  vtkLogScopeFunction(TRACE);
  this->ResetQueues();
}

void vtkSynchronousEncoderDelegate::TerminateInternal()
{
  vtkLogScopeFunction(TRACE);
  this->ResetQueues();
}

void vtkSynchronousEncoderDelegate::PushWorkUnitInternal(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  vtkSmartPointer<vtkRawVideoFrame> dstFrame;
  if (this->BufferSize <= 0)
  {
    this->PrepareFrameInternal(frame, dstFrame);
    this->Frames.push(dstFrame);
  }
  else if (this->Frames.size() == this->BufferSize)
  {
    vtkLog(ERROR, << "Internal buffers are full! Either flush or fetch results with GetResult to "
                     "empty buffers.")
  }
  else if (this->Frames.size() < this->BufferSize)
  {
    this->PrepareFrameInternal(frame, dstFrame);
    this->Frames.push(dstFrame);
  }
}

VTKVideoEncoderResultType vtkSynchronousEncoderDelegate::GetResultInternal()
{
  vtkLogScopeFunction(TRACE);
  VTKVideoEncoderResultType result;
  if (this->Frames.empty())
  {
    vtkLog(ERROR, << "Frames queue is empty.");
    result.first = VTKVideoProcessingStatusType::VTKVPStatus_EOFError;
    result.second = {};
    return result;
  }
  else
  {
    auto frame = this->Frames.front();
    this->Frames.pop();
    result = this->Worker(frame);
    return result;
  }
}
