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
#include "vtkCUDADriverAPI.h"
#include "vtkCUDADriverLoader.h"
#include "vtkCompressedVideoPacket.h"
#include "vtkLogger.h"
#include "vtkNvEncoderInternals.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkTextureObject.h"
#include "vtk_glad.h"

#include <chrono>
#include <memory>
#include <vector>

#define VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(call)                                                \
  do                                                                                               \
  {                                                                                                \
    vtkLogF(TRACE, "CUDRV %s", #call);                                                             \
    auto cufns = this->CUDADriverLoader->FunctionsList;                                            \
    status = cufns->call;                                                                          \
    if (status != CUDA_SUCCESS)                                                                    \
    {                                                                                              \
      const char* szErrName = NULL;                                                                \
      cufns->cuGetErrorName(status, &szErrName);                                                   \
      vtkLogF(ERROR, "CUDA driver API %s failed with error: %s", #call, szErrName);                \
    }                                                                                              \
  } while (0)

class vtkNvEncoderGL::vtkCUDAContext
{
public:
  CUcontext Context;
  std::vector<CUgraphicsResource> Resources;
};

namespace
{
GUID presetMap[] = { NV_ENC_PRESET_P1_GUID, NV_ENC_PRESET_P2_GUID, NV_ENC_PRESET_P3_GUID,
  NV_ENC_PRESET_P4_GUID, NV_ENC_PRESET_P5_GUID, NV_ENC_PRESET_P6_GUID, NV_ENC_PRESET_P7_GUID };

GUID profileMap[] = { NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID, NV_ENC_H264_PROFILE_BASELINE_GUID,
  NV_ENC_H264_PROFILE_MAIN_GUID, NV_ENC_H264_PROFILE_HIGH_GUID, NV_ENC_H264_PROFILE_HIGH_444_GUID,
  NV_ENC_H264_PROFILE_STEREO_GUID, NV_ENC_H264_PROFILE_PROGRESSIVE_HIGH_GUID,
  NV_ENC_H264_PROFILE_CONSTRAINED_HIGH_GUID, NV_ENC_HEVC_PROFILE_MAIN_GUID,
  NV_ENC_HEVC_PROFILE_MAIN10_GUID, NV_ENC_HEVC_PROFILE_FREXT_GUID };
}

vtkStandardNewMacro(vtkNvEncoderGL);

//------------------------------------------------------------------------------
vtkNvEncoderGL::vtkNvEncoderGL()
  : Internals(std::unique_ptr<vtkNvEncoderInternals>(new vtkNvEncoderInternals()))
  , CUDADriverLoader(std::unique_ptr<vtkCUDADriverLoader>(new vtkCUDADriverLoader()))
  , CUDAInstance(std::unique_ptr<vtkCUDAContext>(new vtkCUDAContext()))
{
  this->ResourceCallback =
    new vtkOpenGLResourceFreeCallback<vtkNvEncoderGL>(this, &vtkNvEncoderGL::ReleaseGLResources);
  // Default to a codec this backend supports (H.264, its internal default), so an instance
  // created without an explicit SetCodec is valid. The base class default is VP9, which
  // NVENC does not support.
  this->Codec = VTKVideoCodecType::VTKVC_H264;
}

//------------------------------------------------------------------------------
vtkNvEncoderGL::~vtkNvEncoderGL()
{
  this->Shutdown();
  delete this->ResourceCallback;
}

//------------------------------------------------------------------------------
vtkIdType vtkNvEncoderGL::GetLastEncodeTimeNS() const noexcept
{
  return this->Internals->dtEncode.count();
}

//------------------------------------------------------------------------------
vtkIdType vtkNvEncoderGL::GetLastScaleTimeNS() const noexcept
{
  return this->Internals->dtUpload.count();
}

//------------------------------------------------------------------------------
bool vtkNvEncoderGL::SupportsCodec(VTKVideoCodecType codec) const noexcept
{
  switch (codec)
  {
    case VTKVideoCodecType::VTKVC_H264:
    case VTKVideoCodecType::VTKVC_H265:
      return true;
    default:
      return false;
  }
}

bool vtkNvEncoderGL::CheckAvailability() noexcept // assumes no exception can occur
{
  const auto verbosity = vtkLogger::GetCurrentVerbosityCutoff();
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_OFF);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->OffScreenRenderingOn();
  if (!renderWindow->SupportsOpenGL())
  {
    return false;
  }
  renderWindow->Render();
  vtkNew<vtkNvEncoderGL> encoder;
  encoder->SetCodec(VTKVC_H264); // most wildly available format
  encoder->SetInputPixelFormat(VTKPF_IYUV);
  encoder->SetWidth(640);
  encoder->SetHeight(480);
  encoder->SetGraphicsContext(renderWindow);
  const bool success = encoder->Initialize();

  vtkLogger::SetStderrVerbosity(verbosity);
  return success;
}

//------------------------------------------------------------------------------
bool vtkNvEncoderGL::InitializeInternal()
{
  // 1. Load CUDA driver API from scratch if needed.
  CUresult status;
  if (!this->CUDADriverAvailable)
  {
    this->CUDADriverAvailable = this->CUDADriverLoader->LoadFunctionsTable();
    if (!this->CUDADriverAvailable)
    {
      return false;
    }

    auto cufns = this->CUDADriverLoader->FunctionsList;
    status = cufns->cuInit(0);
    if (status != CUDA_SUCCESS)
    {
      vtkLog(WARNING, "CUDA Driver API unavailable.");
      return false;
    }

    int numGPU = 0;
    status = cufns->cuDeviceGetCount(&numGPU);
    if (status != CUDA_SUCCESS)
    {
      vtkLog(WARNING, "NVIDIA CUDA devices unavailable.");
      return false;
    }

    CUdevice devices[4] = {};
    unsigned int cudaDeviceCount = 0;
    VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(
      cuGLGetDevices_v2(&cudaDeviceCount, devices, 4, CU_GL_DEVICE_LIST_ALL));
    char devName[100];
    if (!cudaDeviceCount)
    {
      vtkLogF(ERROR, "OpenGL rendering is not on a CUDA device.");
      return false;
    }
    else
    {
      status = cufns->cuDeviceGetName(devName, sizeof(devName), devices[0]);
      auto& ctx = this->CUDAInstance->Context;
      ctx = nullptr;
      status = cufns->cuCtxCreate_v2(&ctx, 0, devices[0]);
      unsigned int version = 0;
      cufns->cuCtxGetApiVersion(ctx, &version);
      unsigned int major = version / 1000;
      unsigned int minor = version - major * 1000;
      vtkLogF(TRACE,
        "NVENC GPU #%d ('%s') CUDA ctx %d.%d in use. %d gpu (s) capable of cuda-gl interop.", 0,
        devName, major, minor, cudaDeviceCount);
    }
  }

  // 2. Initializes NVENC with CUDA device.
  auto& internals = (*this->Internals);
  bool success = internals.OpenEncodeSession(NV_ENC_DEVICE_TYPE_CUDA, this->CUDAInstance->Context,
    this->Width, this->Height, vtkNvEncoderInternals::ParsePixelFormat(this->InputPixelFormat));

  if (!success)
  {
    return false;
  }

  auto nvCodec = NV_ENC_CODEC_H264_GUID;
  if (this->Codec == VTKVideoCodecType::VTKVC_H265)
  {
    nvCodec = NV_ENC_CODEC_HEVC_GUID;
  }
  else if (!this->SupportsCodec(this->Codec))
  {
    vtkLogF(ERROR, "Unsupported codec : %s", vtkVideoCodecTypeUtilities::ToString(this->Codec));
    return false;
  }
  auto nvProfile = ::profileMap[this->Profile - 1];
  auto nvPreset = ::presetMap[this->Preset - 1];
  auto nvTuneInfo = static_cast<NV_ENC_TUNING_INFO>(this->Tune);
  if (nvTuneInfo == NV_ENC_TUNING_INFO_LOSSLESS)
  {
    vtkLogF(WARNING, "vtkNvEncoderGL does not support lossless YUV 4:4:4 encoding.");
  }

  NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
  NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
  initializeParams.encodeConfig = &encodeConfig;
  internals.CreateDefaultEncoderInitializeParams(
    &initializeParams, nvCodec, nvPreset, nvProfile, nvTuneInfo);
  vtkNvEncoderInternals::TweakFromEncoderObject(&initializeParams, this);
  this->Initialized = internals.InitializeEncodeCtx(&initializeParams);
  std::string out = internals.FullParamToString(&initializeParams);
  vtkLog(TRACE, << out);
  return this->Initialized;
}

//------------------------------------------------------------------------------
void vtkNvEncoderGL::ShutdownInternal()
{
  this->ResourceCallback->Release();
  this->Internals->Shutdown();
  if (this->Initialized)
  {
    this->CUDADriverLoader->FunctionsList->cuCtxDestroy_v2(this->CUDAInstance->Context);
    this->CUDADriverAvailable = false;
  }
}

//------------------------------------------------------------------------------
bool vtkNvEncoderGL::SetupEncoderFrame(int width, int height)
{
  vtkLogScopeF(TRACE, "%s size=%dx%d", __func__, width, height);
  return this->AllocateInputBuffers();
}

//------------------------------------------------------------------------------
void vtkNvEncoderGL::TearDownEncoderFrame()
{
  this->ReleaseInputBuffers();
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkNvEncoderGL::EncodeInternal(vtkSmartPointer<vtkRawVideoFrame> frame)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = (*this->Internals);
  auto input = this->Internals->GetNextInputFrame();
  auto tu1 = std::chrono::high_resolution_clock::now();
  input->DeepCopy(frame);
  glFlush();
  auto tu2 = std::chrono::high_resolution_clock::now();
  internals.dtUpload = tu2 - tu1;

  auto te1 = std::chrono::high_resolution_clock::now();

  CUresult status;
  auto& ctx = this->CUDAInstance->Context;
  auto glContext = vtkOpenGLRenderWindow::SafeDownCast(this->GraphicsContext);
  if (!glContext)
  {
    vtkLogF(ERROR, "GraphicsContext not set or not an OpenGL render window");
    return { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} };
  }
  glContext->MakeCurrent();
  VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(cuCtxPushCurrent_v2(ctx));
  const auto bfrIdx = internals.NvEncSendCounter % internals.NvEncBufferCount;
  VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(
    cuGraphicsMapResources(1, &this->CUDAInstance->Resources[bfrIdx], nullptr));
  VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(cuCtxPopCurrent_v2(nullptr));
  glContext->ReleaseCurrent();
  auto sstatus = internals.Send(this->ForceIFrame);
  glContext->MakeCurrent();
  VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(cuCtxPushCurrent_v2(ctx));
  VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(
    cuGraphicsUnmapResources(1, &this->CUDAInstance->Resources[bfrIdx], nullptr));
  VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(cuCtxPopCurrent_v2(nullptr));
  glContext->ReleaseCurrent();
  if (sstatus != NV_ENC_SUCCESS)
  {
    return { vtkNvEncoderInternals::ParseNvEncodeAPIStatus(sstatus), {} };
  }
  std::vector<vtkSmartPointer<vtkCompressedVideoPacket>> packets;
  bool success = internals.Receive(packets);
  for (const auto& pkt : packets)
  {
    pkt->SetIsKeyFrame(this->ForceIFrame);
  }
  auto te2 = std::chrono::high_resolution_clock::now();
  internals.dtEncode = te2 - te1;

  if (success)
  {
    return { VTKVideoProcessingStatusType::VTKVPStatus_Success, packets };
  }
  else
  {
    return { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, packets };
  }
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkNvEncoderGL::SendEOS()
{
  vtkLogScopeFunction(TRACE);
  auto& internals = (*this->Internals);
  internals.SendEOS();
  VTKVideoEncoderResultType result;
  internals.Receive(result.second);
  result.first = VTKVideoProcessingStatusType::VTKVPStatus_Success;
  return result;
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
  if (!this->CUDADriverAvailable)
  {
    return false;
  }
  auto glContext = vtkOpenGLRenderWindow::SafeDownCast(this->GraphicsContext);
  if (!glContext)
  {
    vtkLogF(ERROR, "GraphicsContext not set or not an OpenGL render window");
    return false;
  }

  CUresult status;
  auto& ctx = this->CUDAInstance->Context;
  VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(cuCtxPushCurrent_v2(ctx));

  std::vector<void*> inputResources;
  std::vector<vtkSmartPointer<vtkRawVideoFrame>> inputFrames;
  this->ResourceCallback->RegisterGraphicsResources(glContext);
  for (std::size_t i = 0; i < internals.GetEncoderBufferCount(); ++i)
  {
    auto frame = vtk::TakeSmartPointer(vtkOpenGLVideoFrame::New());
    frame->SetWidth(this->Width);
    frame->SetHeight(this->Height);
    frame->SetPixelFormat(this->InputPixelFormat);
    frame->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
    frame->ComputeDefaultStrides();
    frame->SetContext(glContext);
    frame->AllocateDataStore();

    auto vtkTexture = reinterpret_cast<vtkTextureObject*>(frame->GetResourceHandle());
    const GLuint handle = vtkTexture->GetHandle();
    const GLenum target = vtkTexture->GetTarget();
    vtkLogF(TRACE, "handle=%d, target=%d", handle, target);

    CUgraphicsResource resource = nullptr;
    VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(cuGraphicsGLRegisterImage(&resource, handle, target,
      CU_GRAPHICS_REGISTER_FLAGS_READ_ONLY | CU_GRAPHICS_REGISTER_FLAGS_SURFACE_LDST));
    if (status != CUDA_SUCCESS)
    {
      return false;
    }

    this->CUDAInstance->Resources.push_back(resource);
    VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(
      cuGraphicsResourceSetMapFlags(resource, CU_GRAPHICS_MAP_RESOURCE_FLAGS_READ_ONLY));
    if (status != CUDA_SUCCESS)
    {
      return false;
    }

    VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(cuGraphicsMapResources(1, &resource, nullptr));
    if (status != CUDA_SUCCESS)
    {
      return false;
    }

    CUarray deviceArray = nullptr;
    VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(
      cuGraphicsSubResourceGetMappedArray(&deviceArray, resource, 0, 0));
    if (status != CUDA_SUCCESS)
    {
      return false;
    }

    inputResources.push_back(reinterpret_cast<void*>(deviceArray));
    inputFrames.emplace_back(frame);
  }
  VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(cuCtxPopCurrent_v2(nullptr));

  const auto bufFmt = vtkNvEncoderInternals::ParsePixelFormat(this->InputPixelFormat);
  bool success = internals.RegisterInputResources(
    inputResources, inputFrames, NV_ENC_INPUT_RESOURCE_TYPE_CUDAARRAY, bufFmt);
  for (std::size_t i = 0; i < internals.GetEncoderBufferCount(); ++i)
  {
    VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(
      cuGraphicsUnmapResources(1, &this->CUDAInstance->Resources[i], nullptr));
  }

  return success;
}

//------------------------------------------------------------------------------
void vtkNvEncoderGL::ReleaseInputBuffers()
{
  vtkLogScopeFunction(TRACE);
  return this->ReleaseGLResources(this->GraphicsContext);
}

//------------------------------------------------------------------------------
void vtkNvEncoderGL::ReleaseGLResources(vtkWindow* window)
{
  vtkLogScopeFunction(TRACE);
  auto& internals = (*this->Internals);

  if (!internals.IsInitialized())
  {
    return;
  }

  internals.UnregisterInputResources();

  CUresult status;
  auto& ctx = this->CUDAInstance->Context;
  VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(cuCtxPushCurrent_v2(ctx));

  window->MakeCurrent();
  auto& frames = internals.NvEncInputFrames;
  auto& resources = internals.NvEncInputResources;
  for (std::size_t i = 0; i < frames.size(); ++i)
  {
    auto deviceArray = reinterpret_cast<CUarray>(resources[i]);
    auto resource = this->CUDAInstance->Resources[i];
    if (deviceArray != nullptr && resource != nullptr)
    {
      VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(cuGraphicsUnregisterResource(resource));
      if (status != CUDA_SUCCESS)
      {
        vtkLog(ERROR, "Failed to unregister CUDA gfx resource.");
      }
    }
    if (auto glFrame = vtkOpenGLVideoFrame::SafeDownCast(frames[i]))
    {
      glFrame->ReleaseGraphicsResources();
    }
  }
  VTK_NV_CUDA_DRIVER_API_CHECKED_INVOKE(cuCtxPopCurrent_v2(nullptr));
  this->CUDAInstance->Resources.clear();
  internals.NvEncInputFrames.clear();
  internals.NvEncInputResources.clear();
}

//------------------------------------------------------------------------------
// TEMP (VTK 9.6): self-register this backend with vtkEncoderFactory so it can be
// selected by preferences. Delete this block for VTK 9.7 and instead return these
// attributes from vtkNvEncoderGL::CreateOverrideAttributes().
#include "vtkEncoderFactory.h"
namespace
{
vtkVideoEncoder* CreateNvEncoderGL()
{
  return vtkNvEncoderGL::New();
}

// The NvEncode module is built for Linux and Windows, so the Platform attribute is
// resolved at compile time rather than hard-coded.
#if defined(_WIN32)
constexpr const char* kNvEncodePlatform = "Windows";
#elif defined(__APPLE__)
constexpr const char* kNvEncodePlatform = "macOS";
#else
constexpr const char* kNvEncodePlatform = "Linux";
#endif

struct vtkNvEncoderGLRegistrar
{
  vtkNvEncoderGLRegistrar()
  {
    vtkEncoderFactory::BackendDescriptor d;
    d.SubclassName = "vtkNvEncoderGL";
    d.Create = &CreateNvEncoderGL;
    d.Available = &vtkNvEncoderGL::CheckAvailability;
    d.Hardware = true;
    d.Codecs = { VTKVideoCodecType::VTKVC_H264, VTKVideoCodecType::VTKVC_H265 };
    d.Attributes = { { "Platform", kNvEncodePlatform }, { "Hardware", "true" } };
    vtkEncoderFactory::RegisterBackend(d);
  }
};
// Runs when the vtkStreamingNvEncode library is loaded (e.g. on `import vtk_streaming`).
const vtkNvEncoderGLRegistrar sNvEncoderGLRegistrar;
} // anonymous namespace
