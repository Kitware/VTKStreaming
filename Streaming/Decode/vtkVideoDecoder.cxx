/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoDecoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVideoDecoder.h"
#include "vtkAsynchronousDecoderDelegate.h"
#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkVideoDecoder::vtkVideoDecoder() = default;

//------------------------------------------------------------------------------
vtkVideoDecoder::~vtkVideoDecoder()
{
  if (this->HasDelegate())
  {
    this->Delegate->Terminate();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "UsingAsynchronousDelegate: " << (this->HasDelegate() ? "yes" : "no") << '\n';
  if (this->HasDelegate())
  {
    this->Delegate->PrintSelf(os, indent.GetNextIndent());
  }
  os << "Codec: " << vtkVideoCodecTypeUtilities::ToString(this->Codec) << '\n';
  os << "BufferSize: " << this->BufferSize << '\n';
  os << "Initialized: " << this->Initialized << '\n';
}

//------------------------------------------------------------------------------
bool vtkVideoDecoder::HasDelegate()
{
  return this->Delegate != nullptr;
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::SetUseAsynchronousDelegate(bool val)
{
  vtkLogScopeF(TRACE, "%s val=%s", __func__, val ? "true" : "false");
  if (!this->SupportsAsynchronousDelegate())
  {
    vtkLog(TRACE, << "Bypass the delegate.");
    return;
  }

  if (val)
  {
    if (this->HasDelegate())
    {
      return;
    }
    else if (this->Initialized)
    {
      this->Shutdown();
    }
    this->Delegate = vtkAsynchronousDecoderDelegate::New();
    this->Delegate->SetBufferSize(-1);
    this->Delegate->SetNumberOfTasks(1);
    this->Delegate->SetStrictOrdering(true);
  }
  else
  {
    vtkLog(TRACE, << "Bypass the delegate.");
    if (this->HasDelegate())
    {
      this->Shutdown();
      this->Delegate->Delete();
      this->Delegate = nullptr;
      return;
    }
  }
}

//------------------------------------------------------------------------------
bool vtkVideoDecoder::GetUseAsynchronousDelegate()
{
  return this->HasDelegate();
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::UseAsynchronousDelegateOn()
{
  this->SetUseAsynchronousDelegate(true);
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::UseAsynchronousDelegateOff()
{
  this->SetUseAsynchronousDelegate(false);
}

//------------------------------------------------------------------------------
bool vtkVideoDecoder::Initialize()
{
  vtkLogScopeFunction(TRACE);

  if (this->Initialized)
  {
    vtkLog(WARNING,
      "Decoder context already initialized. Please close existing contexts. Call Shutdown()");
    return true;
  }

  this->Initialized = this->InitializeInternal();

  if (this->HasDelegate())
  {
    this->Delegate->Terminate();
    // set the worker function to delegate processing of packets with an async Decoder delegate
    using namespace std::placeholders; // for _1
    VTKVideoDecodeWorkerType worker = std::bind(&vtkVideoDecoder::Decode, this, _1);

    // from this point on, async delegate takes care of processing packets into uncompressed frames.
    this->Delegate->InitializeWorker(worker);
    return this->Initialized;
  }
  else
  {
    // Initialization complete.
    return this->Initialized;
  }
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::Shutdown()
{
  vtkLogScopeFunction(TRACE);
  if (!this->Initialized)
  {
    return;
  }
  if (this->HasDelegate())
  {
    this->CancelPendingDecodeRequests();
    this->Delegate->Terminate();
  }
  this->ShutdownInternal();
  this->Initialized = false;
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::Flush()
{
  vtkLogScopeFunction(TRACE);
  if (!this->Initialized)
  {
    return;
  }
  if (this->HasDelegate())
  {
    // Prevent race conditions - flush pending tasks before draining the encoder.
    this->Delegate->Flush();
  }
  this->FlushInternal();
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkVideoDecoder::Push(vtkCompressedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);

  if (!this->Initialized)
  {
    if (!this->Initialize())
    {
      vtkLog(ERROR, "Failed to initialize decoding context.");
      return VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    }
  }
  this->IgnoreDecodeRequest = false;
  if (this->HasDelegate())
  {
    this->Delegate->PushWorkUnit(packet);
    return VTKVideoProcessingStatusType::VTKVPStatus_Success;
  }
  else
  {
    return this->PushInternal(packet);
  }
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkVideoDecoder::Decode(vtkCompressedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  auto result = this->DecodeInternal(packet);
  this->InvokeEvent(vtkCommand::ProgressEvent);
  return result;
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkVideoDecoder::Drain()
{
  if (!this->Initialized)
  {
    return { VTKVideoProcessingStatusType::VTKVPStatus_Success, {} };
  }
  if (this->HasDelegate())
  {
    // Prevent race conditions - flush pending tasks before draining the encoder.
    this->Delegate->Flush();
  }
  return this->DrainInternal();
}

//------------------------------------------------------------------------------
bool vtkVideoDecoder::HasResult()
{
  vtkLogScopeFunction(TRACE);
  return this->HasDelegate() ? this->Delegate->HasResult() : false;
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkVideoDecoder::GetResult()
{
  auto result = this->HasDelegate() ? this->Delegate->GetResult() : this->GetResultInternal();
  return result;
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::CancelPendingDecodeRequests()
{
  this->IgnoreDecodeRequest = true;
}
