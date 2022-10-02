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
#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkVideoDecoder::vtkVideoDecoder() = default;

//------------------------------------------------------------------------------
vtkVideoDecoder::~vtkVideoDecoder() = default;

//------------------------------------------------------------------------------
void vtkVideoDecoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Codec: " << vtkVideoCodecTypeUtilities::ToString(this->Codec) << '\n';
  os << "Initialized: " << this->Initialized << '\n';
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::SetGraphicsContext(vtkRenderWindow* context)
{
  this->GraphicsContext = context;
}

//------------------------------------------------------------------------------
vtkRenderWindow* vtkVideoDecoder::GetGraphicsContext() const
{
  return this->GraphicsContext;
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
  return this->Initialized;
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::Shutdown()
{
  vtkLogScopeFunction(TRACE);
  if (!this->Initialized)
  {
    return;
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
  return this->PushInternal(packet);
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
  return this->DrainInternal();
}

//------------------------------------------------------------------------------
bool vtkVideoDecoder::HasResult()
{
  vtkLogScopeFunction(TRACE);
  return false;
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkVideoDecoder::GetResult()
{
  auto result = this->GetResultInternal();
  return result;
}
