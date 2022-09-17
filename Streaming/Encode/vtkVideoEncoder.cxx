/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVideoEncoder.h"

#include "vtkAsynchronousEncoderDelegate.h"
#include "vtkEncoderDelegate.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRawVideoFrame.h"
#include "vtkSynchronousEncoderDelegate.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <chrono>
#include <cstddef>

//------------------------------------------------------------------------------
vtkVideoEncoder::vtkVideoEncoder() = default;

//------------------------------------------------------------------------------
vtkVideoEncoder::~vtkVideoEncoder()
{
  if (this->HasDelegate())
  {
    this->Delegate->Terminate();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "BypassDelegate : " << (this->BypassDelegate ? "yes" : "no") << '\n';
  if (!this->BypassDelegate)
  {
    os << "UsingSynchronousDelegate: "
       << (this->Delegate->IsA("vtkSynchronousEncoderDelegate") ? "yes" : "no") << '\n';
    os << "UsingAsynchronousDelegate: "
       << (this->Delegate->IsA("vtkAsynchronousEncoderDelegate") ? "yes" : "no") << '\n';
    this->Delegate->PrintSelf(os, indent.GetNextIndent());
  }
  os << "Codec: " << vtkVideoCodecTypeUtilities::ToString(this->Codec) << '\n';
  os << "NumberOfEncoderThreads: " << this->NumberOfEncoderThreads << '\n';

  os << "Width: " << this->Width << '\n';
  os << "Height: " << this->Height << '\n';
  os << "InputPixelFormat: " << vtkPixelFormatTypeUtilities::ToString(this->InputPixelFormat)
     << '\n';

  os << "GOP size: " << this->GroupOfPicturesSize << '\n';
  os << "Max B-frames: " << this->MaximumBFrames << '\n';
  os << "TimeBaseStart: " << this->TimeBaseStart << '\n';
  os << "TimeBaseEnd: " << this->TimeBaseEnd << '\n';
  os << "BitRate: " << this->BitRate << '\n';
  os << "MaxBitRate: " << this->MaxBitRate << '\n';
  os << "MinBitRate: " << this->MinBitRate << '\n';

  os << "ForceCBR: " << this->ForceCBR << '\n';
  os << "ForceIFrame: " << this->ForceIFrame << '\n';

  os << "Initialized: " << this->Initialized << '\n';
  os << "LastSetupMTime: " << this->LastSetupMTime << '\n';
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::HasDelegate()
{
  return this->Delegate != nullptr;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::UseAsynchronousDelegate()
{
  vtkLogScopeFunction(TRACE);
  if (this->BypassDelegate)
  {
    vtkLog(TRACE, << "Bypass the delegate.");
    return;
  }
  if (!this->SupportsAsynchronousDelegate())
  {
    vtkLog(TRACE, << "Bypass the delegate since encoder does not support asynchronous delegate.");
    this->BypassDelegateOn();
    return;
  }
  if (this->HasDelegate() && !this->Delegate->IsA("vtkAsynchronousEncoderDelegate"))
  {
    this->Shutdown();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
  else if (this->HasDelegate() && this->Delegate->IsA("vtkAsynchronousEncoderDelegate"))
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
void vtkVideoEncoder::UseSynchronousDelegate()
{
  vtkLogScopeFunction(TRACE);
  if (this->BypassDelegate)
  {
    vtkLog(TRACE, << "Bypass the delegate.");
    return;
  }
  if (!this->SupportsSynchronousDelegate())
  {
    vtkLog(TRACE, << "Bypass the delegate since encoder does not support synchronous delegate.");
    this->BypassDelegateOn();
    return;
  }
  if (this->HasDelegate() && !this->Delegate->IsA("vtkSynchronousEncoderDelegate"))
  {
    this->Shutdown();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
  else if (this->HasDelegate() && this->Delegate->IsA("vtkSynchronousEncoderDelegate"))
  {
    return;
  }
  this->Delegate = vtkSynchronousEncoderDelegate::New();
  this->Delegate->SetBufferSize(this->TimeBaseEnd > 1 ? this->TimeBaseEnd / 2 : 1);
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::Initialize()
{
  vtkLogScopeFunction(TRACE);

  if (this->Initialized)
  {
    vtkLog(WARNING,
      "Encoder context already initialized. Please close existing contexts. Call Shutdown()");
    return true;
  }

  this->Initialized = this->InitializeInternal();

  if (this->BypassDelegate && this->HasDelegate())
  {
    // do not use the delegate, terminate existing and delete it.
    this->Delegate->Terminate();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
  else if (!this->HasDelegate() && !this->BypassDelegate)
  {
    // create default delegate if we don't have one but need one.
    this->UseSynchronousDelegate();
  }
  if (this->BypassDelegate)
  {
    // Initialization complete.
    return this->Initialized;
  }
  else
  {
    // set the worker function to delegate processing of frames to the encoder delegate
    using namespace std::placeholders; // for _1
    VTKVideoEncodeWorkerType worker = std::bind(&vtkVideoEncoder::EncodeInternal, this, _1);

    // from this point on, delegate takes care of processing frames to compressed packets.
    this->Delegate->InitializeWorker(worker);
    return this->Initialized;
  }
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::Shutdown()
{
  vtkLogScopeFunction(TRACE);
  if (!this->Initialized)
  {
    return;
  }
  if (!this->BypassDelegate && this->HasDelegate())
  {
    this->Delegate->Terminate();
  }
  this->ShutdownInternal();
  this->Initialized = false;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::Flush()
{
  vtkLogScopeFunction(TRACE);
  if (!this->BypassDelegate)
  {
    this->Delegate->Flush();
  }
  this->FlushInternal();
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetBypassDelegate(bool val)
{
  vtkLogScopeFunction(TRACE);
  if (val != this->BypassDelegate)
  {
    this->BypassDelegate = val;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::GetBypassDelegate()
{
  vtkLogScopeFunction(TRACE);
  return this->BypassDelegate;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::BypassDelegateOn()
{
  vtkLogScopeFunction(TRACE);
  this->SetBypassDelegate(true);
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::BypassDelegateOff()
{
  vtkLogScopeFunction(TRACE);
  this->SetBypassDelegate(false);
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetForceLowLatency(bool val)
{
  vtkLogScopeFunction(TRACE);
  this->ForceLowLatency = val;
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::GetForceLowLatency()
{
  vtkLogScopeFunction(TRACE);
  return this->ForceLowLatency;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::ForceLowLatencyOn()
{
  vtkLogScopeFunction(TRACE);
  this->ForceLowLatency = true;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::ForceLowLatencyOff()
{
  vtkLogScopeFunction(TRACE);
  this->ForceLowLatency = false;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetKeyFramesOnly(bool val)
{
  vtkLogScopeFunction(TRACE);
  this->KeyFramesOnly = val;
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::GetKeyFramesOnly()
{
  vtkLogScopeFunction(TRACE);
  return this->KeyFramesOnly;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::KeyFramesOnlyOn()
{
  vtkLogScopeFunction(TRACE);
  this->KeyFramesOnly = true;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::KeyFramesOnlyOff()
{
  vtkLogScopeFunction(TRACE);
  this->KeyFramesOnly = false;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetForceCBR(bool val)
{
  vtkLogScopeFunction(TRACE);
  if (val != this->ForceCBR)
  {
    this->ForceCBR = val;
    this->MaxBitRate = this->MinBitRate = this->BitRate;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::GetForceCBR()
{
  vtkLogScopeFunction(TRACE);
  return this->ForceCBR;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::ForceCBROn()
{
  vtkLogScopeFunction(TRACE);
  this->SetForceCBR(true);
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::ForceCBROff()
{
  vtkLogScopeFunction(TRACE);
  this->SetForceCBR(false);
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetForceIFrame(bool val)
{
  vtkLogScopeFunction(TRACE);
  this->ForceIFrame = val;
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::GetForceIFrame()
{
  vtkLogScopeFunction(TRACE);
  return this->ForceIFrame;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::ForceIFrameOn()
{
  vtkLogScopeFunction(TRACE);
  this->ForceIFrame = true;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::ForceIFrameOff()
{
  vtkLogScopeFunction(TRACE);
  this->ForceIFrame = false;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVideoEncoder::Drain()
{
  vtkLogScopeFunction(TRACE);
  return this->DrainInternal();
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkVideoEncoder::Push(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);

  if (frame == nullptr)
  {
    return VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
  }

  const int& width = frame->GetWidth();
  const int& height = frame->GetHeight();

  bool success = true;
  // check if we've to setup a new frame.
  if (this->NeedsNewEncoderFrame(width, height) || this->LastSetupMTime < this->GetMTime())
  {
    // When the dimensions change, a new context is required. Otherwise, a listening decoder will be
    // oblivious to the change in dimensions. Some encoders are capable of dynamic resizing but
    // they're few.
    this->Shutdown();
    success = this->Initialize();
    if (!success)
    {
      return VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    }

    success = this->SetupEncoderFrame(width, height);
    this->LastSetupMTime = success ? this->GetMTime() : -1;

    if (!success)
    {
      vtkLog(ERROR, << "Failed to setup an encoder frame");
      return VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    }
  }
  else if (!this->Initialized)
  {
    if (!this->Initialize())
    {
      vtkLog(ERROR, "Failed to initialize encoding context.");
      return VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    }
    success = this->SetupEncoderFrame(width, height);
    this->LastSetupMTime = success ? this->GetMTime() : -1;

    if (!success)
    {
      vtkLog(ERROR, << "Failed to setup an encoder frame");
      return VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    }
  }

  if (this->BypassDelegate ||
    (!this->SupportsAsynchronousDelegate() && !this->SupportsSynchronousDelegate()))
  {
    return this->PushInternal(frame);
  }
  else
  {
    this->Delegate->PushWorkUnit(frame);
    return VTKVideoProcessingStatusType::VTKVPStatus_Success;
  }
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::HasResult()
{
  vtkLogScopeFunction(TRACE);
  return this->BypassDelegate ? false : this->Delegate->HasResult();
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVideoEncoder::GetResult()
{
  vtkLogScopeFunction(TRACE);
  auto result = this->BypassDelegate ? this->GetResultInternal() : this->Delegate->GetResult();
  return result;
}
