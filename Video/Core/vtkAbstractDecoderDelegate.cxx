#include "vtkAbstractDecoderDelegate.h"
#include "vtkLogger.h"

vtkAbstractDecoderDelegate::vtkAbstractDecoderDelegate() = default;

vtkAbstractDecoderDelegate::~vtkAbstractDecoderDelegate() = default;

void vtkAbstractDecoderDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

void vtkAbstractDecoderDelegate::InitializeWorker(DecodeWorkerType workerFunc)
{
  vtkLogScopeFunction(TRACE);
  return this->InitializeWorkerInternal(workerFunc);
}

void vtkAbstractDecoderDelegate::Flush()
{
  vtkLogScopeFunction(TRACE);
  return this->FlushInternal();
}

void vtkAbstractDecoderDelegate::Terminate()
{
  vtkLogScopeFunction(TRACE);
  return this->TerminateInternal();
}

void vtkAbstractDecoderDelegate::PushWorkUnit(vtkCodedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);
  return this->PushWorkUnitInternal(packet);
}

DecoderResultType vtkAbstractDecoderDelegate::GetResult()
{
  vtkLogScopeFunction(TRACE);
  return this->GetResultInternal();
}

void vtkAbstractDecoderDelegate::PreparePacketInternal(
  vtkCodedVideoPacket* packet, DecoderInputType& dstPacket)
{
  vtkLogScopeFunction(TRACE);
  dstPacket.TakeReference(vtkCodedVideoPacket::New());
  dstPacket->ShallowCopy(packet);
  dstPacket->AllocateCopy(packet);

  // copy data from all planes.
  unsigned char* srcData = nullptr;
  unsigned char* dstData = nullptr;
  const int srcSize = packet->GetData(srcData);
  if (srcSize > 0)
  {
    const int dstSize = dstPacket->GetData(dstData);
    assert(dstSize == srcSize);
    dstPacket->CopyData(srcData, srcSize);
  }
}