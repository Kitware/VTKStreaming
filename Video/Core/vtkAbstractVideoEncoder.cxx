/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractVideoEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractVideoEncoder.h"

#include "vtkAbstractEncoderDelegate.h"
#include "vtkAsynchronousEncoderDelegate.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkRawVideoFrame.h"
#include "vtkSynchronousEncoderDelegate.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <chrono>
#include <cstddef>

//------------------------------------------------------------------------------
vtkAbstractVideoEncoder::vtkAbstractVideoEncoder() = default;

//------------------------------------------------------------------------------
vtkAbstractVideoEncoder::~vtkAbstractVideoEncoder()
{
  if (this->Delegate != nullptr)
  {
    this->Delegate->Terminate();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::UseAsynchronousDelegate()
{
  vtkLogScopeFunction(TRACE);
  if (this->Delegate != nullptr && !this->Delegate->IsA("vtkAsynchronousEncoderDelegate"))
  {
    this->Shutdown();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
  else if (this->Delegate != nullptr && this->Delegate->IsA("vtkAsynchronousEncoderDelegate"))
  {
    return;
  }
  auto asyncDelegate = vtkAsynchronousEncoderDelegate::New();
  asyncDelegate->SetBufferSize(-1);
  asyncDelegate->SetNumberOfTasks(1);
  asyncDelegate->SetStrictOrdering(true);
  this->Delegate = asyncDelegate;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::UseSynchronousDelegate()
{
  vtkLogScopeFunction(TRACE);
  if (this->Delegate != nullptr && !this->Delegate->IsA("vtkSynchronousEncoderDelegate"))
  {
    this->Shutdown();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
  else if (this->Delegate != nullptr && this->Delegate->IsA("vtkSynchronousEncoderDelegate"))
  {
    return;
  }
  this->Delegate = vtkSynchronousEncoderDelegate::New();
  this->Delegate->SetBufferSize(this->TimeBaseEnd > 1 ? this->TimeBaseEnd / 2 : 1);
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

//------------------------------------------------------------------------------
bool vtkAbstractVideoEncoder::Initialize()
{
  vtkLogScopeFunction(TRACE);

  if (this->Initialized)
  {
    vtkLog(WARNING,
      "Encoder context already initialized. Please close existing contexts. Call Shutdown()");
    return true;
  }

  this->Initialized = this->InitializeInternal();

  // create default delegate if we don't have one.
  if (this->Delegate == nullptr)
  {
    this->UseSynchronousDelegate();
  }
  // set the worker function to delegate processing of frames to the encoder delegate
  using namespace std::placeholders; // for _1
  EncodeWorkerType worker = std::bind(&vtkAbstractVideoEncoder::EncodeInternal, this, _1);

  // from this point on, delegate takes care of processing frames to compressed packets.
  this->Delegate->InitializeWorker(worker);
  return this->Initialized;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::Shutdown()
{
  vtkLogScopeFunction(TRACE);
  if (!this->Initialized)
  {
    return;
  }
  this->Delegate->Terminate();
  this->ShutdownInternal();
  this->Initialized = false;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::Flush()
{
  this->Delegate->Flush();
  this->FlushInternal();
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::SetForceCBR(bool val)
{
  this->ForceCBR = val;
  this->MaxBitRate = this->MinBitRate = this->BitRate;
}

//------------------------------------------------------------------------------
bool vtkAbstractVideoEncoder::GetForceCBR()
{
  return this->ForceCBR;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::ForceCBROn()
{
  this->SetForceCBR(true);
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::ForceCBROff()
{
  this->SetForceCBR(false);
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::SetForceIFrame(bool val)
{
  this->ForceIFrame = val;
}

//------------------------------------------------------------------------------
bool vtkAbstractVideoEncoder::GetForceIFrame()
{
  return this->ForceIFrame;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::ForceIFrameOn()
{
  this->ForceIFrame = true;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::ForceIFrameOff()
{
  this->ForceIFrame = false;
}

//------------------------------------------------------------------------------
bool vtkAbstractVideoEncoder::Push(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);

  const int& width = frame->GetWidth();
  const int& height = frame->GetHeight();

  if (frame != nullptr)
  {
    vtkLog(TRACE, << "Sending frame - pts=" << frame->GetPresentationTS());
  }

  bool success = true;
  // check if we've to setup a new frame.
  if (this->NeedsNewEncoderFrame(width, height) || this->LastSetupMTime < this->GetMTime())
  {
    // When the dimensions change, a new context is required. Otherwise, a listening decoder will be
    // oblivious to the change in dimensions.
    this->Shutdown();
    success = this->Initialize();
    if (!success)
    {
      return false;
    }

    // this resets frame->pts = 0
    success = this->SetupEncoderFrame(width, height);
    this->LastSetupMTime = success ? this->GetMTime() : -1;

    if (!success)
    {
      vtkLog(ERROR, << "Failed to setup an encoder frame");
      return false;
    }
  }
  else if (!this->Initialized)
  {
    if (!this->Initialize())
    {
      vtkLog(ERROR, "Failed to initialize encoding context.");
      return false;
    }
  }
  this->Delegate->PushWorkUnit(frame);
  return true;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::Drain()
{
  return this->DrainInternal();
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkAbstractVideoEncoder::GetResult(
  vtkSmartPointer<vtkCodedVideoPacket>& packet)
{
  auto result = this->Delegate->GetResult();
  packet = result.second;
  return result.first;
}
