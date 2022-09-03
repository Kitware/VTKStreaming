#include "vtkAbstractEncoderDelegate.h"
#include "vtkLogger.h"

vtkAbstractEncoderDelegate::vtkAbstractEncoderDelegate() = default;

vtkAbstractEncoderDelegate::~vtkAbstractEncoderDelegate() = default;

void vtkAbstractEncoderDelegate::PrintSelf(ostream &os, vtkIndent indent)
{
  (void) os;
  (void) indent;
}

void vtkAbstractEncoderDelegate::InitializeWorker(EncodeWorkerType workerFunc)
{
  vtkLogScopeFunction(TRACE);
  return this->InitializeWorkerInternal(workerFunc);
}

void vtkAbstractEncoderDelegate::Flush()
{
  vtkLogScopeFunction(TRACE);
  return this->FlushInternal();
}

void vtkAbstractEncoderDelegate::Terminate()
{
  vtkLogScopeFunction(TRACE);
  return this->TerminateInternal();
}

void vtkAbstractEncoderDelegate::PushWorkUnit(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  return this->PushWorkUnitInternal(frame);
}

EncoderResultType vtkAbstractEncoderDelegate::GetResult()
{
  vtkLogScopeFunction(TRACE);
  return this->GetResultInternal();
}

void vtkAbstractEncoderDelegate::PrepareFrameInternal(
  vtkRawVideoFrame* frame, EncoderInputType& dstFrame)
{
  vtkLogScopeFunction(TRACE);
  dstFrame.TakeReference(vtkRawVideoFrame::New());
  dstFrame->ShallowCopy(frame);
  dstFrame->AllocateCopy(frame);

  // copy data from all planes.
  for (int i = 0; i < VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES; ++i)
  {
    unsigned char* srcData = nullptr;
    unsigned char* dstData = nullptr;
    const int srcSize = frame->GetData(srcData, i);
    if (srcSize > 0)
    {
      const int dstSize = dstFrame->GetData(dstData);
      assert(dstSize == srcSize);
      dstFrame->CopyData(srcData, srcSize, i);
    }
  }
}