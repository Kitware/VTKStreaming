#include "vtkAsynchronousDecoderDelegate.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkAsynchronousDecoderDelegate);

vtkAsynchronousDecoderDelegate::vtkAsynchronousDecoderDelegate()
  : TaskQueue(nullptr)
  , TrySucceeded(false)
{
}

vtkAsynchronousDecoderDelegate::~vtkAsynchronousDecoderDelegate() = default;

void vtkAsynchronousDecoderDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

void vtkAsynchronousDecoderDelegate::InitializeWorkerInternal(DecodeWorkerType workerFunc)
{
  this->TaskQueue.reset(new TaskQueueType(
    workerFunc, this->StrictOrdering, this->BufferSize, static_cast<int>(this->NumberOfTasks)));
}

void vtkAsynchronousDecoderDelegate::FlushInternal()
{
  if (this->TaskQueue != nullptr)
  {
    this->TaskQueue->Flush();
  }
}

void vtkAsynchronousDecoderDelegate::TerminateInternal()
{
  vtkLogScopeFunction(TRACE);
  if (this->TaskQueue != nullptr)
  {
    vtkLog(TRACE, << "Flushing the task queue ...");
    this->TaskQueue->Flush();
  }
  this->TaskQueue.reset(nullptr);
}

void vtkAsynchronousDecoderDelegate::PushWorkUnitInternal(vtkCodedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  vtkSmartPointer<vtkCodedVideoPacket> dstPacket;
  this->PreparePacketInternal(packet, dstPacket);
  this->TaskQueue->Push(std::move(dstPacket));
}

bool vtkAsynchronousDecoderDelegate::HasResult()
{
  DecoderResultType result;
  this->TrySucceeded = this->TaskQueue->TryPop(result);
  return this->TrySucceeded;
}

DecoderResultType vtkAsynchronousDecoderDelegate::GetResultInternal()
{
  vtkLogScopeFunction(TRACE);
  if (this->TrySucceeded)
  {
    // reset it first.
    this->TrySucceeded = false;
    // the result already exists, give it out.
    return this->Result;
  }
  else if (this->TaskQueue->IsEmpty())
  {
    vtkLog(WARNING, << "Need packets. Task queue empty.")
  }
  this->TaskQueue->Pop(this->Result);
  return this->Result;
}