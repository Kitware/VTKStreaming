/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMockVideoDecoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMockVideoDecoder.h"
#include "vtkCompressedVideoPacket.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <chrono>
#include <thread>

vtkStandardNewMacro(vtkMockVideoDecoder);

//------------------------------------------------------------------------------
vtkMockVideoDecoder::vtkMockVideoDecoder() = default;

//------------------------------------------------------------------------------
vtkMockVideoDecoder::~vtkMockVideoDecoder()
{
  this->Shutdown();
}

//------------------------------------------------------------------------------
void vtkMockVideoDecoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkIdType vtkMockVideoDecoder::GetLastDecodeTimeNS() const noexcept
{
  return this->MockDecodeInterval;
}

//------------------------------------------------------------------------------
vtkIdType vtkMockVideoDecoder::GetLastScaleTimeNS() const noexcept
{
  return 0;
}

//------------------------------------------------------------------------------
bool vtkMockVideoDecoder::InitializeInternal()
{
  return true;
}

//------------------------------------------------------------------------------
void vtkMockVideoDecoder::ShutdownInternal() {}

//------------------------------------------------------------------------------
void vtkMockVideoDecoder::FlushInternal() {}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkMockVideoDecoder::PushInternal(vtkCompressedVideoPacket* packet)
{
  return VTKVideoProcessingStatusType::VTKVPStatus_Success;
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkMockVideoDecoder::GetResultInternal()
{
  return VTKVideoDecoderResultType({ VTKVideoProcessingStatusType::VTKVPStatus_Success, {} });
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkMockVideoDecoder::DecodeInternal(vtkCompressedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  if (this->PacketCounter % this->MockLargePacketPeriod)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(this->MockDecodeInterval));
  }
  else
  {

    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->MockDecodeInterval * this->MockLargePacketIntervalRatio));
  }
  auto data = packet->GetData();
  this->PacketCounter++;
  return { VTKVideoProcessingStatusType::VTKVPStatus_Success, {} };
}

//------------------------------------------------------------------------------
VTKVideoDecoderResultType vtkMockVideoDecoder::DrainInternal()
{
  return { VTKVideoProcessingStatusType::VTKVPStatus_Success, {} };
}
