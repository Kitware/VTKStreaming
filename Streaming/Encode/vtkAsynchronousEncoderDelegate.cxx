/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAsynchronousEncoderDelegate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAsynchronousEncoderDelegate.h"
#include "vtkCPUVideoFrame.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkAsynchronousEncoderDelegate);

vtkAsynchronousEncoderDelegate::vtkAsynchronousEncoderDelegate()
  : TaskQueue(nullptr)
  , TrySucceeded(false)
{
}

vtkAsynchronousEncoderDelegate::~vtkAsynchronousEncoderDelegate() = default;

void vtkAsynchronousEncoderDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "NumberOfTasks: " << this->NumberOfTasks << "\n";
  os << "StrictOrdering: " << this->StrictOrdering << "\n";
  os << "TrySucceeded: " << this->TrySucceeded << "\n";
  os << "TaskQueueIsEmpty: " << this->TaskQueue->IsEmpty() << "\n";
}

void vtkAsynchronousEncoderDelegate::InitializeWorker(VTKVideoEncodeWorkerType workerFunc)
{
  this->TaskQueue.reset(new TaskQueueType(
    workerFunc, this->StrictOrdering, this->BufferSize, static_cast<int>(this->NumberOfTasks)));
}

void vtkAsynchronousEncoderDelegate::Flush()
{
  if (this->TaskQueue != nullptr)
  {
    this->TaskQueue->Flush();
  }
}

void vtkAsynchronousEncoderDelegate::Terminate()
{
  vtkLogScopeFunction(TRACE);
  if (this->TaskQueue != nullptr)
  {
    vtkLog(TRACE, << "Flushing the task queue ...");
    this->TaskQueue->Flush();
  }
  this->TaskQueue.reset(nullptr);
}

void vtkAsynchronousEncoderDelegate::PushWorkUnit(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  vtkSmartPointer<vtkRawVideoFrame> dstFrame;
  this->PrepareFrame(frame, dstFrame);
  this->TaskQueue->Push(std::move(dstFrame));
}

bool vtkAsynchronousEncoderDelegate::HasResult()
{
  this->TrySucceeded = this->TaskQueue->TryPop(this->Result);
  return this->TrySucceeded;
}

VTKVideoEncoderResultType vtkAsynchronousEncoderDelegate::GetResult()
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
    vtkLog(WARNING, << "Need frames. Task queue empty.")
  }
  this->TaskQueue->Pop(this->Result);
  return this->Result;
}

void vtkAsynchronousEncoderDelegate::PrepareFrame(
  vtkRawVideoFrame* frame, VTKVideoEncoderInputType& dstFrame)
{
  vtkLogScopeFunction(TRACE);
  dstFrame = vtk::TakeSmartPointer(vtkCPUVideoFrame::New());
  dstFrame->DeepCopy(frame);
}
