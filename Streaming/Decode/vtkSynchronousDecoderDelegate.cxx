/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronousDecoderDelegate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSynchronousDecoderDelegate.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSynchronousDecoderDelegate);

vtkSynchronousDecoderDelegate::vtkSynchronousDecoderDelegate() = default;

vtkSynchronousDecoderDelegate::~vtkSynchronousDecoderDelegate() = default;

void vtkSynchronousDecoderDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "No. of inputs : " << this->Packets.size() << '\n';
  os << "No. of results : " << this->Results.size() << '\n';
}

void vtkSynchronousDecoderDelegate::ResetQueues()
{
  vtkLogScopeFunction(TRACE);
  auto packetQueue = std::queue<VTKVideoDecoderInputType>();
  this->Packets.swap(packetQueue);

  auto resultsQueue = std::queue<VTKVideoDecoderResultType>();
  this->Results.swap(resultsQueue);
}

void vtkSynchronousDecoderDelegate::InitializeWorkerInternal(VTKVideoDecodeWorkerType workerFunc)
{
  vtkLogScopeFunction(TRACE);
  this->Worker = workerFunc;
  this->ResetQueues();
}

void vtkSynchronousDecoderDelegate::FlushInternal()
{
  vtkLogScopeFunction(TRACE);
  this->ResetQueues();
}

void vtkSynchronousDecoderDelegate::TerminateInternal()
{
  vtkLogScopeFunction(TRACE);
  this->ResetQueues();
}

void vtkSynchronousDecoderDelegate::PushWorkUnitInternal(vtkCompressedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  vtkSmartPointer<vtkCompressedVideoPacket> dstPacket;
  if (this->BufferSize <= 0)
  {
    this->PreparePacketInternal(packet, dstPacket);
    this->Packets.push(dstPacket);
  }
  else if (this->Packets.size() == this->BufferSize)
  {
    vtkLog(ERROR, << "Internal buffers are full! Either flush or fetch results with GetResult to "
                     "empty buffers.")
  }
  else if (this->Packets.size() < this->BufferSize)
  {
    this->PreparePacketInternal(packet, dstPacket);
    this->Packets.push(dstPacket);
  }
}

VTKVideoDecoderResultType vtkSynchronousDecoderDelegate::GetResultInternal()
{
  vtkLogScopeFunction(TRACE);
  VTKVideoDecoderResultType result;
  if (this->Packets.empty())
  {
    vtkLog(ERROR, << "Packets queue is empty.");
    result.first = VTKVideoProcessingStatusType::VTKVPStatus_EOFError;
    result.second = {};
    return result;
  }
  else
  {
    auto packet = this->Packets.front();
    this->Packets.pop();
    result = this->Worker(packet);
    return result;
  }
}
