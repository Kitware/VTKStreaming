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
#include "vtkCodedVideoPacket.h"

#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

//------------------------------------------------------------------------------
vtkAbstractVideoDecoder::vtkAbstractVideoDecoder() = default;

//------------------------------------------------------------------------------
vtkAbstractVideoDecoder::~vtkAbstractVideoDecoder() = default;

//------------------------------------------------------------------------------
void vtkAbstractVideoDecoder::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
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
  this->ShutdownInternal();
}

//------------------------------------------------------------------------------
void vtkAbstractVideoDecoder::Flush()
{
  vtkLogScopeFunction(TRACE);
  this->FlushInternal();
}

//------------------------------------------------------------------------------
bool vtkAbstractVideoDecoder::Push(vtkCodedVideoPacket* pkt)
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
  return this->PushInternal(pkt);
}
