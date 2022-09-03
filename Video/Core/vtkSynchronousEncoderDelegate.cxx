#include "vtkSynchronousEncoderDelegate.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSynchronousEncoderDelegate);

vtkSynchronousEncoderDelegate::vtkSynchronousEncoderDelegate() = default;

vtkSynchronousEncoderDelegate::~vtkSynchronousEncoderDelegate() = default;

void vtkSynchronousEncoderDelegate::PrintSelf(ostream &os, vtkIndent indent)
{
  (void) os;
  (void) indent;
}

void vtkSynchronousEncoderDelegate::ResetQueues()
{
  vtkLogScopeFunction(TRACE);
  auto frameQueue = std::queue<EncoderInputType>();
  this->Frames.swap(frameQueue);

  auto resultsQueue = std::queue<EncoderResultType>();
  this->Results.swap(resultsQueue);
}

void vtkSynchronousEncoderDelegate::InitializeWorkerInternal(EncodeWorkerType workerFunc)
{
  vtkLogScopeFunction(TRACE);
  this->Worker = workerFunc;
  this->ResetQueues();
}

void vtkSynchronousEncoderDelegate::FlushInternal()
{
  vtkLogScopeFunction(TRACE);
  this->ResetQueues();
}

void vtkSynchronousEncoderDelegate::TerminateInternal()
{
  vtkLogScopeFunction(TRACE);
  this->ResetQueues();
}

void vtkSynchronousEncoderDelegate::PushWorkUnitInternal(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  vtkSmartPointer<vtkRawVideoFrame> dstFrame;
  if (this->BufferSize <= 0)
  {
    this->PrepareFrameInternal(frame, dstFrame);
    this->Frames.push(dstFrame);
  }
  else if (this->Frames.size() == this->BufferSize)
  {
    vtkLog(ERROR, << "Internal buffers are full! Either flush or fetch results with GetResult to "
                     "empty buffers.")
  }
  else if (this->Frames.size() < this->BufferSize)
  {
    this->PrepareFrameInternal(frame, dstFrame);
    this->Frames.push(dstFrame);
  }
}

EncoderResultType vtkSynchronousEncoderDelegate::GetResultInternal()
{
  vtkLogScopeFunction(TRACE);
  EncoderResultType result;
  if (this->Frames.empty())
  {
    vtkLog(ERROR, << "Frames queue is empty.");
    result.first = VTKVideoProcessingStatusType::EOFError;
    result.second = nullptr;
    return result;
  }
  else
  {
    auto frame = this->Frames.front();
    this->Frames.pop();
    result = this->Worker(frame);
    return result;
  }
}