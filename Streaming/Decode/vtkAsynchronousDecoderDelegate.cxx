/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAsynchronousDecoderDelegate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAsynchronousDecoderDelegate.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkAsynchronousDecoderDelegate);

vtkAsynchronousDecoderDelegate::vtkAsynchronousDecoderDelegate()
  : TaskQueue(nullptr)
  , TrySucceeded(false)
{
}

vtkAsynchronousDecoderDelegate::~vtkAsynchronousDecoderDelegate() = default;

void vtkAsynchronousDecoderDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "NumberOfTasks: " << this->NumberOfTasks << "\n";
  os << "StrictOrdering: " << this->StrictOrdering << "\n";
  os << "TrySucceeded: " << this->TrySucceeded << "\n";
  os << "TaskQueueIsEmpty: " << this->TaskQueue->IsEmpty() << "\n";
}

void vtkAsynchronousDecoderDelegate::InitializeWorkerInternal(VTKVideoDecodeWorkerType workerFunc)
{
  this->TaskQueue.reset(new TaskQueueType(
    workerFunc, this->StrictOrdering, this->BufferSize, static_cast<int>(this->NumberOfTasks)));
}

void vtkAsynchronousDecoderDelegate::FlushInternal()
{
  if (this->TaskQueue != nullptr)
  {
    this->TaskQueue->Flush();
  }
}

void vtkAsynchronousDecoderDelegate::TerminateInternal()
{
  vtkLogScopeFunction(TRACE);
  if (this->TaskQueue != nullptr)
  {
    vtkLog(TRACE, << "Flushing the task queue ...");
    this->TaskQueue->Flush();
  }
  this->TaskQueue.reset(nullptr);
}

void vtkAsynchronousDecoderDelegate::PushWorkUnitInternal(vtkCompressedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  vtkSmartPointer<vtkCompressedVideoPacket> dstPacket;
  this->PreparePacketInternal(packet, dstPacket);
  this->TaskQueue->Push(std::move(dstPacket));
}

bool vtkAsynchronousDecoderDelegate::HasResult()
{
  this->TrySucceeded = this->TaskQueue->TryPop(this->Result);
  return this->TrySucceeded;
}

VTKVideoDecoderResultType vtkAsynchronousDecoderDelegate::GetResultInternal()
{
  vtkLogScopeFunction(TRACE);
  if (this->TrySucceeded)
  {
    // reset it first.
    this->TrySucceeded = false;
    // the result already exists, give it out.
    return this->Result;
  }
  else if (this->TaskQueue->IsEmpty())
  {
    vtkLog(WARNING, << "Need packets. Task queue empty.")
  }
  this->TaskQueue->Pop(this->Result);
  return this->Result;
}
