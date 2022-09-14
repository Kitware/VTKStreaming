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
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSynchronousDecoderDelegate.h"

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
  os << "BufferSize: " << this->BufferSize << '\n';
  os << "Initialized: " << this->Initialized << '\n';
}

//------------------------------------------------------------------------------
bool vtkVideoDecoder::HasDelegate()
{
  return this->Delegate != nullptr;
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::UseAsynchronousDelegate()
{
  vtkLogScopeFunction(TRACE);
  if (this->BypassDelegate)
  {
    vtkLog(TRACE, << "Bypassing the delegate.");
    return;
  }
  if (this->HasDelegate() && !this->Delegate->IsA("vtkAsynchronousDecoderDelegate"))
  {
    this->Shutdown();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
  else if (this->HasDelegate() && this->Delegate->IsA("vtkAsynchronousDecoderDelegate"))
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
void vtkVideoDecoder::UseSynchronousDelegate()
{
  vtkLogScopeFunction(TRACE);
  if (this->BypassDelegate)
  {
    vtkLog(TRACE, << "Bypassing the delegate.");
    return;
  }
  if (this->HasDelegate() && !this->Delegate->IsA("vtkSynchronousDecoderDelegate"))
  {
    this->Shutdown();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
  else if (this->HasDelegate() && this->Delegate->IsA("vtkSynchronousDecoderDelegate"))
  {
    return;
  }
  this->Delegate = vtkSynchronousDecoderDelegate::New();
  this->Delegate->SetBufferSize(this->BufferSize);
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
    // set the worker function to delegate processing of packets to the encoder delegate
    using namespace std::placeholders; // for _1
    VTKVideoDecodeWorkerType worker = std::bind(&vtkVideoDecoder::DecodeInternal, this, _1);

    // from this point on, delegate takes care of processing packets to compressed packets.
    this->Delegate->InitializeWorker(worker);
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
  if (!this->BypassDelegate && this->HasDelegate())
  {
    this->Delegate->Terminate();
  }
  this->ShutdownInternal();
  this->Initialized = false;
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::Flush()
{
  vtkLogScopeFunction(TRACE);
  if (this->HasDelegate())
  {
    this->Delegate->Flush();
  }
  this->FlushInternal();
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::SetBypassDelegate(bool val)
{
  vtkLogScopeFunction(TRACE);
  if (val != this->BypassDelegate)
  {
    this->BypassDelegate = val;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkVideoDecoder::GetBypassDelegate()
{
  vtkLogScopeFunction(TRACE);
  return this->BypassDelegate;
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::BypassDelegateOn()
{
  vtkLogScopeFunction(TRACE);
  this->SetBypassDelegate(true);
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::BypassDelegateOff()
{
  vtkLogScopeFunction(TRACE);
  this->SetBypassDelegate(false);
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
  if (this->BypassDelegate)
  {
    return this->PushInternal(packet);
  }
  else
  {
    this->Delegate->PushWorkUnit(packet);
    return VTKVideoProcessingStatusType::VTKVPStatus_Success;
  }
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::Drain()
{
  this->DrainInternal();
}

//------------------------------------------------------------------------------
bool vtkVideoDecoder::HasResult()
{
  vtkLogScopeFunction(TRACE);
  return this->BypassDelegate ? false : this->Delegate->HasResult();
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkVideoDecoder::GetResult()
{
  auto result = this->BypassDelegate ? this->GetResultInternal() : this->Delegate->GetResult();
  return result;
}
