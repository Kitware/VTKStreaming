/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractVideoDecoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractVideoDecoder.h"
#include "vtkAsynchronousDecoderDelegate.h"
#include "vtkCodedVideoPacket.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkRawVideoFrame.h"
#include "vtkSynchronousDecoderDelegate.h"
#include "vtkVideoProcessingStatusTypes.h"

//------------------------------------------------------------------------------
vtkAbstractVideoDecoder::vtkAbstractVideoDecoder() = default;

//------------------------------------------------------------------------------
vtkAbstractVideoDecoder::~vtkAbstractVideoDecoder()
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
void vtkAbstractVideoDecoder::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoDecoder::UseAsynchronousDelegate()
{
  vtkLogScopeFunction(TRACE);
  if (this->Delegate != nullptr && !this->Delegate->IsA("vtkAsynchronousDecoderDelegate"))
  {
    this->Shutdown();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
  else if (this->Delegate != nullptr && this->Delegate->IsA("vtkAsynchronousDecoderDelegate"))
  {
    return;
  }
  auto asyncDelegate = vtkAsynchronousDecoderDelegate::New();
  asyncDelegate->SetBufferSize(-1);
  asyncDelegate->SetNumberOfTasks(1);
  asyncDelegate->SetStrictOrdering(true);
  this->Delegate = asyncDelegate;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoDecoder::UseSynchronousDelegate()
{
  vtkLogScopeFunction(TRACE);
  if (this->Delegate != nullptr && !this->Delegate->IsA("vtkSynchronousDecoderDelegate"))
  {
    this->Shutdown();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
  else if (this->Delegate != nullptr && this->Delegate->IsA("vtkSynchronousDecoderDelegate"))
  {
    return;
  }
  this->Delegate = vtkSynchronousDecoderDelegate::New();
  this->Delegate->SetBufferSize(this->BufferSize);
}

//------------------------------------------------------------------------------
bool vtkAbstractVideoDecoder::Initialize()
{
  vtkLogScopeFunction(TRACE);

  if (this->Initialized)
  {
    vtkLog(WARNING,
      "Decoder context already initialized. Please close existing contexts. Call Shutdown()");
    return true;
  }
  this->Initialized = this->InitializeInternal();

  // create default delegate if we don't have one.
  if (this->Delegate == nullptr)
  {
    this->UseSynchronousDelegate();
  }
  // set the worker function to delegate processing of frames to the decoder delegate
  using namespace std::placeholders; // for _1
  DecodeWorkerType worker = std::bind(&vtkAbstractVideoDecoder::DecodeInternal, this, _1);

  // from this point on, delegate takes care of processing packets to uncompressed images.
  this->Delegate->InitializeWorker(worker);
  return this->Initialized;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoDecoder::Shutdown()
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
void vtkAbstractVideoDecoder::Flush()
{
  vtkLogScopeFunction(TRACE);
  this->Delegate->Flush();
  this->FlushInternal();
}

//------------------------------------------------------------------------------
bool vtkAbstractVideoDecoder::Push(vtkCodedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);

  if (!this->Initialized)
  {
    if (!this->Initialize())
    {
      vtkLog(ERROR, "Failed to initialize decoding context.");
      return false;
    }
  }
  this->Delegate->PushWorkUnit(packet);
  return true;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoDecoder::Drain()
{
  this->DrainInternal();
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkAbstractVideoDecoder::GetResult(
  vtkSmartPointer<vtkRawVideoFrame>& packet)
{
  auto result = this->Delegate->GetResult();
  packet = result.second;
  return result.first;
}