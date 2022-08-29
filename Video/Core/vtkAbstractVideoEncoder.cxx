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
#include "vtkCodedVideoPacket.h"
#include "vtkRawVideoFrame.h"

#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include <chrono>
#include <cstddef>

//------------------------------------------------------------------------------
vtkAbstractVideoEncoder::vtkAbstractVideoEncoder() = default;

//------------------------------------------------------------------------------
vtkAbstractVideoEncoder::~vtkAbstractVideoEncoder() = default;

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
  this->ShutdownInternal();
  this->Initialized = false;
}

//------------------------------------------------------------------------------
void vtkAbstractVideoEncoder::Flush()
{
  vtkLogScopeFunction(TRACE);
  this->FlushInternal();
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
void vtkAbstractVideoEncoder::PacketHandler(vtkCodedVideoPacket* pkt)
{
  vtkLogScopeFunction(TRACE);
  this->InvokeEvent(vtkCommand::ProgressEvent, reinterpret_cast<void*>(pkt));
}

//------------------------------------------------------------------------------
bool vtkAbstractVideoEncoder::Push(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);

  if (!this->Initialized)
  {
    if (!this->Initialize())
    {
      vtkLog(ERROR, "Failed to initialize encoding context.");
      return false;
    }
  }

  const int& w = frame->GetWidth();
  const int& h = frame->GetHeight();

  bool success = true;
  // check if we've to setup a new frame.
  if (this->NeedsNewEncoderFrame(w, h) || this->LastSetupMTime < this->GetMTime())
  {
    this->TearDownEncoderFrame();
    // this resets frame->pts = 0
    success = this->SetupEncoderFrame(w, h);
    this->LastSetupMTime = success ? this->GetMTime() : -1;

    if (!success)
    {
      vtkLog(ERROR, << "Failed to setup an encoder frame");
      return false;
    }
  }

  return this->PushInternal(frame);
}
