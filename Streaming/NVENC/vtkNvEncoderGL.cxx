/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNvEncoderGL.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNvEncoderGL.h"
#include "nvEncodeAPI.h"
#include "vtkCompressedVideoPacket.h"
#include "vtkLogger.h"
#include "vtkNvEncoderInternals.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkRawVideoFrame.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkType.h"
#include "vtkVideoCodecTypes.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtk_glew.h"

#include <GL/gl.h>
#include <memory>
#include <vector>

vtkStandardNewMacro(vtkNvEncoderGL);

//------------------------------------------------------------------------------
vtkNvEncoderGL::vtkNvEncoderGL()
  : Internals(std::unique_ptr<vtkNvEncoderInternals>(new vtkNvEncoderInternals()))
{
}

//------------------------------------------------------------------------------
vtkNvEncoderGL::~vtkNvEncoderGL()
{
  this->ShutdownInternal();
}

//------------------------------------------------------------------------------
void vtkNvEncoderGL::InitializeOpenGLContext(vtkOpenGLRenderWindow* window)
{
  this->Window = window;
}

//------------------------------------------------------------------------------
vtkIdType vtkNvEncoderGL::GetLastEncodeTimeNS() const noexcept
{
  return 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkNvEncoderGL::GetLastScaleTimeNS() const noexcept
{
  return 0;
}

//------------------------------------------------------------------------------
bool vtkNvEncoderGL::SupportsCodec(VTKVideoCodecType codec) const noexcept
{
  return false;
};

//------------------------------------------------------------------------------
bool vtkNvEncoderGL::InitializeInternal()
{
  if (!this->Internals->OpenEncodeSession(NV_ENC_DEVICE_TYPE_OPENGL, nullptr, this->Width,
        this->Height, vtkNvEncoderInternals::ParsePixelFormat(this->InputPixelFormat)))
  {
    return false;
  }
  else
  {
    NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
    NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
    initializeParams.encodeConfig = &encodeConfig;
    this->Internals->CreateDefaultEncoderInitializeParams(&initializeParams, NV_ENC_CODEC_H264_GUID,
      NV_ENC_PRESET_P3_GUID, NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY);
    vtkNvEncoderInternals::TweakFromEncoderObject(&initializeParams, this);
    this->Initialized = this->Internals->InitializeEncodeCtx(&initializeParams);
    std::string out = this->Internals->FullParamToString(&initializeParams);
    vtkLog(TRACE, << out);
  }
  return this->Initialized;
}

//------------------------------------------------------------------------------
void vtkNvEncoderGL::ShutdownInternal()
{
  this->ReleaseGLResources();
  this->Internals->Shutdown();
  this->Initialized = false;
}

//------------------------------------------------------------------------------
void vtkNvEncoderGL::FlushInternal()
{
  this->Internals->Flush();
}

//------------------------------------------------------------------------------
bool vtkNvEncoderGL::SetupEncoderFrame(const int& width, const int& height)
{
  vtkLogScopeF(TRACE, "%s size=%dx%d", __func__, width, height);
  return this->AllocateInputBuffers();
}

//------------------------------------------------------------------------------
bool vtkNvEncoderGL::NeedsNewEncoderFrame(const int& width, const int& height)
{
  bool outdated = this->Width != width || this->Height != height;
  this->Width = width;
  this->Height = height;
  return outdated;
}

//------------------------------------------------------------------------------
void vtkNvEncoderGL::TearDownEncoderFrame()
{
  this->ReleaseInputBuffers();
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkNvEncoderGL::PushInternal(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = (*this->Internals);
  auto input = this->Internals->GetNextInputFrame();
  if (frame->GetAttachedRenderWindow() == this->Window)
  {
    // single gpu-copy:
    //  'frame' does not have data. grab pixels from it's attached render window instead.
    input->Capture(this->Window);
  }
  else
  {
    // initiate a cpu->gpu upload or a gpu->gpu copy.
    input->CopyFrameData(frame);
  }

  return vtkNvEncoderInternals::ParseNvEncodeAPIStatus(internals.Send(this->ForceIFrame));
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkNvEncoderGL::GetResultInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = (*this->Internals);
  std::vector<vtkSmartPointer<vtkCompressedVideoPacket>> packets;
  bool success = internals.Receive(packets);
  return VTKVideoEncoderResultType(
    { success ? VTKVideoProcessingStatusType::VTKVPStatus_Success
              : VTKVideoProcessingStatusType::VTKVPStatus_UnknownError,
      packets });
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkNvEncoderGL::EncodeInternal(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = (*this->Internals);
  auto input = this->Internals->GetNextInputFrame();
  auto glFrame = vtkOpenGLVideoFrame::SafeDownCast(input);
  if (glFrame == nullptr)
  {
    vtkLog(ERROR, << "Encoder does not have valid input frames. vtkOpenGLVideoFrame");
    return { VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue, {} };
  }
  input->CopyFrameData(frame);
  auto status = internals.Send(this->ForceIFrame);
  std::vector<vtkSmartPointer<vtkCompressedVideoPacket>> packets;
  bool success = internals.Receive(packets);
  return VTKVideoEncoderResultType(
    { success ? VTKVideoProcessingStatusType::VTKVPStatus_Success
              : VTKVideoProcessingStatusType::VTKVPStatus_UnknownError,
      packets });
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkNvEncoderGL::DrainInternal()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = (*this->Internals);
  internals.SendEOS();
  std::vector<vtkSmartPointer<vtkCompressedVideoPacket>> packets;
  auto status = internals.Receive(packets, false);
  return { status ? VTKVideoProcessingStatusType::VTKVPStatus_Success
                  : VTKVideoProcessingStatusType::VTKVPStatus_UnknownError,
    packets };
}

//------------------------------------------------------------------------------
bool vtkNvEncoderGL::AllocateInputBuffers()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = (*this->Internals);

  if (!internals.IsInitialized())
  {
    vtkLogF(ERROR, "Encoder device not initialized %d", NV_ENC_ERR_ENCODER_NOT_INITIALIZED);
    return false;
  }

  std::vector<void*> inputResources;
  std::vector<vtkSmartPointer<vtkRawVideoFrame>> inputFrames;
  unsigned int widthBytes = 0;
  for (std::size_t i = 0; i < internals.GetEncoderBufferCount(); ++i)
  {
    // this will be encoder's  reference to the resource. must free it
    // when releasing resources.
    auto resource = new NV_ENC_INPUT_RESOURCE_OPENGL_TEX;

    auto frame = vtk::TakeSmartPointer(vtkOpenGLVideoFrame::New());
    frame->SetWidth(this->Width);
    frame->SetHeight(this->Height);
    frame->SetPixelFormat(this->InputPixelFormat);
    frame->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
    frame->ComputeDefaultStrides();
    frame->InitializeGraphicsResources(this->Window);
    frame->AllocateDataStore();
    widthBytes = vtkRawVideoFrame::GetWidthBytes(this->Width, this->InputPixelFormat);
    auto vtkTexture = reinterpret_cast<vtkTextureObject*>(frame->GetResourceHandle());
    resource->texture = vtkTexture->GetHandle();
    resource->target = vtkTexture->GetTarget();
    vtkLogF(TRACE, "tex=%d, target=%d", resource->texture, resource->target);

    inputResources.push_back(resource);
    inputFrames.emplace_back(frame);
  }

  const auto bufFmt = vtkNvEncoderInternals::ParsePixelFormat(this->InputPixelFormat);
  bool success = internals.RegisterInputResources(inputResources, inputFrames,
    NV_ENC_INPUT_RESOURCE_TYPE_OPENGL_TEX, this->Width, this->Height, this->Width, bufFmt);
  return success;
}

//------------------------------------------------------------------------------
void vtkNvEncoderGL::ReleaseInputBuffers()
{
  vtkLogScopeFunction(TRACE);
  return this->ReleaseGLResources();
}

//------------------------------------------------------------------------------
void vtkNvEncoderGL::ReleaseGLResources()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = (*this->Internals);

  if (!internals.IsInitialized())
  {
    return;
  }

  internals.UnregisterInputResources();

  auto& frames = internals.NvEncInputFrames;
  auto& resources = internals.NvEncInputResources;
  for (std::size_t i = 0; i < frames.size(); ++i)
  {
    // free the OpenGL resource if we own it.
    if (frames[i]->GetAttachedRenderWindow() == nullptr)
    {
      if (auto glFrame = vtkOpenGLVideoFrame::SafeDownCast(frames[i]))
      {
        glFrame->ReleaseGraphicsResources();
      }
    }
    // free the encoder's reference to the resource.
    auto resource = reinterpret_cast<NV_ENC_INPUT_RESOURCE_OPENGL_TEX*>(resources[i]);
    delete resource;
  }
  internals.NvEncInputFrames.clear();
  internals.NvEncInputResources.clear();
}
