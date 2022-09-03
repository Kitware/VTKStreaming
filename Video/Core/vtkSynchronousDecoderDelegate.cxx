#include "vtkSynchronousDecoderDelegate.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSynchronousDecoderDelegate);

vtkSynchronousDecoderDelegate::vtkSynchronousDecoderDelegate() = default;

vtkSynchronousDecoderDelegate::~vtkSynchronousDecoderDelegate() = default;

void vtkSynchronousDecoderDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

void vtkSynchronousDecoderDelegate::ResetQueues()
{
  vtkLogScopeFunction(TRACE);
  auto packetQueue = std::queue<DecoderInputType>();
  this->Packets.swap(packetQueue);

  auto resultsQueue = std::queue<DecoderResultType>();
  this->Results.swap(resultsQueue);
}

void vtkSynchronousDecoderDelegate::InitializeWorkerInternal(DecodeWorkerType workerFunc)
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

void vtkSynchronousDecoderDelegate::PushWorkUnitInternal(vtkCodedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  vtkSmartPointer<vtkCodedVideoPacket> dstPacket;
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

DecoderResultType vtkSynchronousDecoderDelegate::GetResultInternal()
{
  vtkLogScopeFunction(TRACE);
  DecoderResultType result;
  if (this->Packets.empty())
  {
    vtkLog(ERROR, << "Packets queue is empty.");
    result.first = VTKVideoProcessingStatusType::EOFError;
    result.second = nullptr;
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