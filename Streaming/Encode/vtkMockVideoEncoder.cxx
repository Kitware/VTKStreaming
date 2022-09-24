/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMockVideoEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMockVideoEncoder.h"
#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include <chrono>
#include <thread>

vtkStandardNewMacro(vtkMockVideoEncoder);

//------------------------------------------------------------------------------
vtkMockVideoEncoder::vtkMockVideoEncoder() = default;

//------------------------------------------------------------------------------
vtkMockVideoEncoder::~vtkMockVideoEncoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkMockVideoEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkIdType vtkMockVideoEncoder::GetLastEncodeTimeNS() const noexcept
{
  return this->MockEncodeInterval;
}

//------------------------------------------------------------------------------
vtkIdType vtkMockVideoEncoder::GetLastScaleTimeNS() const noexcept
{
  return 0;
}

//------------------------------------------------------------------------------
bool vtkMockVideoEncoder::InitializeInternal()
{
  return true;
}

//------------------------------------------------------------------------------
void vtkMockVideoEncoder::ShutdownInternal() {}

//------------------------------------------------------------------------------
void vtkMockVideoEncoder::FlushInternal() {}

//------------------------------------------------------------------------------
bool vtkMockVideoEncoder::SetupEncoderFrame(int, int)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkMockVideoEncoder::NeedsNewEncoderFrame(int, int)
{
  return false;
}

//------------------------------------------------------------------------------
void vtkMockVideoEncoder::TearDownEncoderFrame() {}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkMockVideoEncoder::PushInternal(vtkRawVideoFrame* frame)
{
  return VTKVideoProcessingStatusType::VTKVPStatus_Success;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkMockVideoEncoder::GetResultInternal()
{
  return VTKVideoEncoderResultType({ VTKVideoProcessingStatusType::VTKVPStatus_Success, {} });
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkMockVideoEncoder::EncodeInternal(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  if (this->MockLargeFramePeriod && this->FrameCounter % this->MockLargeFramePeriod)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(this->MockEncodeInterval));
  }
  else if (this->MockLargeFramePeriod > 0)
  {
    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->MockEncodeInterval * this->MockLargeFrameIntervalRatio));
  }
  else
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(this->MockEncodeInterval));
  }
  auto data = frame->GetData();
  this->FrameCounter++;
  return VTKVideoEncoderResultType({ VTKVideoProcessingStatusType::VTKVPStatus_Success, {} });
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkMockVideoEncoder::DrainInternal()
{
  return {};
}
