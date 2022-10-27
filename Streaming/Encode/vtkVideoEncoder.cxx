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

#include "vtkLogger.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkVideoEncoder::vtkVideoEncoder() = default;

//------------------------------------------------------------------------------
vtkVideoEncoder::~vtkVideoEncoder() = default;

//------------------------------------------------------------------------------
void vtkVideoEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Encoder Context: \n";
  os << "Codec: " << vtkVideoCodecTypeUtilities::ToString(this->Codec) << '\n';
  os << "Width: " << this->Width << '\n';
  os << "Height: " << this->Height << '\n';
  os << "InputPixelFormat: " << vtkPixelFormatTypeUtilities::ToString(this->InputPixelFormat)
     << '\n';
  os << "KeyFramesOnly: " << this->KeyFramesOnly << '\n';
  os << "LowDelayMode: " << this->LowDelayMode << '\n';
  os << "NumberOfEncoderThreads: " << this->NumberOfEncoderThreads << '\n';

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
void vtkVideoEncoder::SetOutputHandler(std::function<void(VTKVideoEncoderResultType)> outputHandler)
{
  this->OutputHandler = outputHandler;
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
void vtkVideoEncoder::SetCodec(int codec)
{
  vtkLogScopeFunction(TRACE);
  if (codec >= 0 && codec < static_cast<int>(VTKVideoCodecType::VTKVC_MaxNumberOfSupportedCodecs))
  {
    this->SetCodec(static_cast<VTKVideoCodecType>(codec));
  }
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetWidth(int width)
{
  vtkLogScopeF(TRACE, "%s, w=%d", __func__, width);
  this->Width = width;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::SetHeight(int height)
{
  vtkLogScopeF(TRACE, "%s, h=%d", __func__, height);
  this->Height = height;
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
  return this->Initialized;
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::Shutdown()
{
  vtkLogScopeFunction(TRACE);
  if (!this->Initialized)
  {
    return;
  }
  this->Drain();
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
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::Drain()
{
  vtkLogScopeFunction(TRACE);
  if (!this->Initialized)
  {
    return;
  }
  this->SendEOS();
}

//------------------------------------------------------------------------------
void vtkVideoEncoder::Encode(vtkSmartPointer<vtkRawVideoFrame> frame)
{
  vtkLogScopeFunction(TRACE);

  const int& width = frame->GetWidth();
  const int& height = frame->GetHeight();

  auto ctxStatus = this->UpdateEncoderContext(width, height);
  if (ctxStatus != VTKVideoProcessingStatusType::VTKVPStatus_Success)
  {
    vtkLogF(ERROR, "Failed to update encoder context for %dx%d", width, height);
    if (this->OutputHandler != nullptr)
    {
      this->OutputHandler({ ctxStatus, {} });
    }
    else
    {
      this->InvokeEvent(vtkVideoEncoder::EncodedVideoChunkEvent, nullptr);
    }
    return;
  }
  if (this->OutputHandler != nullptr)
  {
    this->OutputHandler(this->EncodeInternal(frame));
  }
  else
  {
    auto result = this->EncodeInternal(frame);
    for (const auto& chunk : result.second)
    {
      this->InvokeEvent(vtkVideoEncoder::EncodedVideoChunkEvent, chunk.GetPointer());
    }
  }
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkVideoEncoder::UpdateEncoderContext(int width, int height)
{
  vtkLogScopeF(TRACE, "%s %dx%d", __func__, width, height);
  bool success = true;
  // check if we've to setup a new frame.
  if (this->NeedsNewEncoderFrame(width, height) || this->LastSetupMTime < this->GetMTime() ||
    !this->Initialized)
  {
    // When the dimensions change, a new context is required. Otherwise, a listening decoder will
    // be oblivious to the change in dimensions. Some encoders are capable of dynamic resizing but
    // they're few.
    this->Shutdown();
    this->LastSetupMTime = this->GetMTime();
    if (!this->Initialize())
    {
      vtkLog(ERROR, "Failed to initialize encoding context.");
      return VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    }
    const bool success = this->SetupEncoderFrame(width, height);
    if (!success)
    {
      vtkLog(ERROR, << "Failed to setup an encoder frame");
      return VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
    }
  }
  return VTKVideoProcessingStatusType::VTKVPStatus_Success;
}
