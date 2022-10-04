/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVideoEncoder.h"

#include "vtkAsynchronousEncoderDelegate.h"
#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

//------------------------------------------------------------------------------
vtkVideoEncoder::vtkVideoEncoder() = default;

//------------------------------------------------------------------------------
vtkVideoEncoder::~vtkVideoEncoder()
{
  if (this->HasDelegate())
  {
    this->Delegate->Terminate();
    this->Delegate->Delete();
    this->Delegate = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "AsyncMode: " << (this->HasDelegate() ? "yes" : "no") << '\n';
  if (this->HasDelegate())
  {
    this->Delegate->PrintSelf(os, indent.GetNextIndent());
  }
  os << "Encoder Context: \n";
  os << "Codec: " << vtkVideoCodecTypeUtilities::ToString(this->Codec) << '\n';
  os << "Width: " << this->Width << '\n';
  os << "Height: " << this->Height << '\n';
  os << "InputPixelFormat: " << vtkPixelFormatTypeUtilities::ToString(this->InputPixelFormat)
     << '\n';
  os << "KeyFramesOnly: " << this->KeyFramesOnly << '\n';
  os << "LowDelayMode: " << this->LowDelayMode << '\n';
  os << "NumberOfEncoderThreads: " << this->NumberOfEncoderThreads << '\n';
  os << "SupportsAsyncMode: " << this->SupportsAsyncMode() << '\n';
  os << "SupportsZeroCopy: " << this->SupportsZeroCopy() << '\n';

  os << "Sequence Parameters: \n";
  os << "GOP size: " << this->GroupOfPicturesSize << '\n';
  os << "Max B-frames: " << this->MaximumBFrames << '\n';
  os << "TimeBaseStart: " << this->TimeBaseStart << '\n';
  os << "TimeBaseEnd: " << this->TimeBaseEnd << '\n';
  os << "ForceIFrame: " << this->ForceIFrame << '\n';

  os << "Bit Rate Control: \n";
  switch (this->BitRateControlMode)
  {
    case vtkVideoEncoder::BRCType::CBR:
      os << "Constant Bit Rate\n";
    case vtkVideoEncoder::BRCType::CQP:
      os << "Constant Quantization Parameter\n";
    case vtkVideoEncoder::BRCType::VBR:
      os << "Variable Bit Rate\n";
    default:
      break;
  }
  os << "BitRate: " << this->BitRate << '\n';
  os << "MaxBitRate: " << this->MaxBitRate << '\n';
  os << "MinBitRate: " << this->MinBitRate << '\n';
  os << "QuantizationParameter: " << this->QuantizationParameter << '\n';

  os << "Initialized: " << this->Initialized << '\n';
  os << "LastSetupMTime: " << this->LastSetupMTime << '\n';
  os << "GraphicsContext: ";
  if (this->GraphicsContext == nullptr)
  {
    os << "(nullptr)\n";
  }
  this->GraphicsContext->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetGraphicsContext(vtkRenderWindow* context)
{
  this->GraphicsContext = context;
}

//------------------------------------------------------------------------------
vtkRenderWindow* vtkVideoEncoder::GetGraphicsContext() const
{
  return this->GraphicsContext;
}

//------------------------------------------------------------------------------
vtkRenderWindow* vtkVideoEncoder::GetDelegateGraphicsContext() const
{
  return this->Delegate ? this->Delegate->GetWorkerGfxContext() : nullptr;
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::HasDelegate()
{
  return this->Delegate != nullptr;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetAsyncMode(bool val)
{
  vtkLogScopeF(TRACE, "%s val=%s", __func__, val ? "true" : "false");
  if (!this->SupportsAsyncMode())
  {
    vtkLog(TRACE, << "Bypass the delegate.");
    return;
  }

  if (val)
  {
    if (this->HasDelegate())
    {
      return;
    }
    else if (this->Initialized)
    {
      this->Shutdown();
    }
    this->Delegate = vtkAsynchronousEncoderDelegate::New();
  }
  else
  {
    vtkLog(TRACE, << "Bypass the delegate.");
    if (this->HasDelegate())
    {
      this->Shutdown();
      this->Delegate->Delete();
      this->Delegate = nullptr;
      return;
    }
  }
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::GetAsyncMode()
{
  return this->HasDelegate();
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::AsyncModeOn()
{
  this->SetAsyncMode(true);
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::AsyncModeOff()
{
  this->SetAsyncMode(false);
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetCodec(int codec)
{
  vtkLogScopeFunction(TRACE);
  if (codec >= 0 && codec < static_cast<int>(VTKVideoCodecType::VTKVC_MaxNumberOfSupportedCodecs))
  {
    this->SetCodec(static_cast<VTKVideoCodecType>(codec));
  }
}
//------------------------------------------------------------------------------
void vtkVideoEncoder::SetInputPixelFormat(int pixFmt)
{
  vtkLogScopeFunction(TRACE);
  if (pixFmt >= 0 && pixFmt <= 3)
  {
    this->SetInputPixelFormat(static_cast<VTKPixelFormatType>(pixFmt));
  }
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::NeedsNewEncoderFrame(int width, int height)
{
  auto alignedW = vtkRawVideoFrame::AlignUp(width, 8);
  auto alignedH = vtkRawVideoFrame::AlignUp(height, 8);
  vtkLogScopeF(TRACE, "%s my=%dx%d | given=%dx%d | aligned=%dx%d", __func__, this->Width,
    this->Height, width, height, alignedW, alignedH);
  bool outdated = alignedW != this->Width || alignedH != this->Height;
  this->Width = alignedW;
  this->Height = alignedH;
  return outdated;
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::Initialize()
{
  vtkLogScopeFunction(TRACE);

  if (this->Initialized)
  {
    vtkLog(WARNING,
      "Encoder context already initialized. Please close existing contexts. Call Shutdown()");
    return true;
  }

  this->Initialized = this->InitializeInternal();

  if (this->HasDelegate())
  {
    this->Delegate->Terminate();
    // set the worker function to delegate processing of frames with an async encoder delegate
    using namespace std::placeholders; // for _1
    VTKVideoEncodeWorkerType worker = std::bind(&vtkVideoEncoder::Encode, this, _1);

    // from this point on, async delegate takes care of processing frames into compressed packets.
    this->Delegate->InitializeWorker(worker, this->GraphicsContext);
    return this->Initialized;
  }
  else
  {
    // Initialization complete.
    return this->Initialized;
  }
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::Shutdown()
{
  vtkLogScopeFunction(TRACE);
  if (!this->Initialized)
  {
    return;
  }
  if (this->HasDelegate())
  {
    this->CancelPendingEncodeRequests();
    this->Delegate->Terminate();
  }
  this->ShutdownInternal();
  this->Initialized = false;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::Flush()
{
  vtkLogScopeFunction(TRACE);
  if (!this->Initialized)
  {
    return;
  }
  if (this->HasDelegate())
  {
    // Prevent race conditions - flush pending tasks before draining the encoder.
    this->Delegate->Flush();
  }
  this->FlushInternal();
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetBitRateControlMode(int mode)
{
  if (mode >= 0 && mode < 3 && mode != static_cast<int>(this->BitRateControlMode))
  {
    this->BitRateControlMode = static_cast<vtkVideoEncoder::BRCType>(mode);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetForceIFrame(bool val)
{
  vtkLogScopeFunction(TRACE);
  this->ForceIFrame = val;
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::GetForceIFrame()
{
  vtkLogScopeFunction(TRACE);
  return this->ForceIFrame;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::ForceIFrameOn()
{
  vtkLogScopeFunction(TRACE);
  this->ForceIFrame = true;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::ForceIFrameOff()
{
  vtkLogScopeFunction(TRACE);
  this->ForceIFrame = false;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVideoEncoder::Drain()
{
  vtkLogScopeFunction(TRACE);
  if (!this->Initialized)
  {
    return { VTKVideoProcessingStatusType::VTKVPStatus_Success, {} };
  }
  if (this->HasDelegate())
  {
    // Prevent race conditions - flush pending tasks before draining the encoder.
    this->Delegate->Flush();
  }
  return this->DrainInternal();
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkVideoEncoder::Push(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);

  if (frame == nullptr)
  {
    return VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
  }

  const int& width = frame->GetWidth();
  const int& height = frame->GetHeight();

  auto ctxStatus = this->UpdateEncoderContext(width, height);
  if (ctxStatus != VTKVideoProcessingStatusType::VTKVPStatus_Success)
  {
    return ctxStatus;
  }

  this->IgnoreEncodeRequest = false;
  if (this->HasDelegate())
  {
    this->Delegate->PushWorkUnit(frame);
    return VTKVideoProcessingStatusType::VTKVPStatus_Success;
  }
  else
  {
    return this->PushInternal(frame);
  }
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVideoEncoder::Encode(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  if (this->IgnoreEncodeRequest)
  {
    vtkLog(TRACE, "Ignoring encode request.");
    return { VTKVideoProcessingStatusType::VTKVPStatus_Success, {} };
  }
  auto result = this->EncodeInternal(frame);
  this->InvokeEvent(vtkCommand::ProgressEvent);
  return result;
}

//------------------------------------------------------------------------------
bool vtkVideoEncoder::HasResult()
{
  vtkLogScopeFunction(TRACE);
  return this->HasDelegate() ? this->Delegate->HasResult() : false;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVideoEncoder::GetResult()
{
  vtkLogScopeFunction(TRACE);
  auto result = this->HasDelegate() ? this->Delegate->GetResult() : this->GetResultInternal();
  return result;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::CancelPendingEncodeRequests()
{
  this->IgnoreEncodeRequest = true;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVideoEncoder::EncodeDisplay()
{
  vtkLogScopeF(TRACE, "%s window=%s", __func__, vtkLogIdentifier(this->GraphicsContext));

  if (this->GraphicsContext == nullptr)
  {
    vtkLog(WARNING,
      "Please set graphics context with "
      "vtkVideoEncoder::SetGraphicsContext(vtkRenderWindow*).");
    return { VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue, {} };
  }
  if (this->GetAsyncMode())
  {
    vtkLog(ERROR, "The encoder is in async mode. Please use Push/GetResult API.");
    return { VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue, {} };
  }
  else
  {
    const int& width = this->GraphicsContext->GetSize()[0];
    const int& height = this->GraphicsContext->GetSize()[1];

    auto ctxStatus = this->UpdateEncoderContext(width, height);
    if (ctxStatus == VTKVideoProcessingStatusType::VTKVPStatus_Success)
    {
      return this->EncodeDisplayInternal();
    }
    else
    {
      return { ctxStatus, {} };
    }
  }
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkVideoEncoder::UpdateEncoderContext(int width, int height)
{
  vtkLogScopeF(TRACE, "%s %dx%d", __func__, width, height);
  bool success = true;
  // check if we've to setup a new frame.
  if (this->NeedsNewEncoderFrame(width, height) || this->LastSetupMTime < this->GetMTime())
  {
    // When the dimensions change, a new context is required. Otherwise, a listening decoder will
    // be oblivious to the change in dimensions. Some encoders are capable of dynamic resizing but
    // they're few.
    this->Shutdown();
    success = this->Initialize();
    if (!success)
    {
      return VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    }

    success = this->SetupEncoderFrame(width, height);
    this->LastSetupMTime = success ? this->GetMTime() : -1;

    if (!success)
    {
      vtkLog(ERROR, << "Failed to setup an encoder frame");
      return VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    }
  }
  else if (!this->Initialized)
  {
    if (!this->Initialize())
    {
      vtkLog(ERROR, "Failed to initialize encoding context.");
      return VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    }
    success = this->SetupEncoderFrame(width, height);
    this->LastSetupMTime = success ? this->GetMTime() : -1;

    if (!success)
    {
      vtkLog(ERROR, << "Failed to setup an encoder frame");
      return VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    }
  }
  return VTKVideoProcessingStatusType::VTKVPStatus_Success;
}
