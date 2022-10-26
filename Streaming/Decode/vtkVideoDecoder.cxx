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
void vtkVideoDecoder::SetOutputHandler(std::function<void(VTKVideoDecoderResultType)> outputHandler)
{
  this->OutputHandler = outputHandler;
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
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::Drain()
{
  vtkLogScopeFunction(TRACE);
  if (!this->Initialized)
  {
    return;
  }
  this->SendEOS();
}

//------------------------------------------------------------------------------
void vtkVideoDecoder::Decode(vtkSmartPointer<vtkCompressedVideoPacket> packet)
{
  vtkLogScopeFunction(TRACE);
  if (!this->UpdateDecoderContext(packet->GetWidth(), packet->GetHeight()))
  {
    if (this->OutputHandler != nullptr)
    {
      this->OutputHandler({ VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} });
    }
    else
    {
      this->InvokeEvent(vtkVideoDecoder::DecodedVideoFrameEvent, nullptr);
    }
  }
  if (this->OutputHandler != nullptr)
  {
    this->OutputHandler(this->DecodeInternal(packet));
  }
  else
  {
    auto result = this->DecodeInternal(packet);
    for (const auto& frame : result.second)
    {
      this->InvokeEvent(vtkVideoDecoder::DecodedVideoFrameEvent, frame.GetPointer());
    }
  }
}

//------------------------------------------------------------------------------
bool vtkVideoDecoder::UpdateDecoderContext(int width, int height)
{
  vtkLogScopeFunction(TRACE);
  if (width != this->Width || height != this->Height || !this->Initialized)
  {
    this->Shutdown();
    this->Width = width;
    this->Height = height;
    if (!this->Initialize())
    {
      vtkLog(ERROR, "Failed to initialize decoding context.");
      return false;
    }
  }
  return true;
}
