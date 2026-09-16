// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "vtkVulkanEncoder.h"

#include "vtkCompressedVideoPacket.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkRawVideoFrame.h"
#include "vtkVulkanEncoderInternals.h"
#include "vtkVulkanGLInterop.h"
#include "vtkVulkanVideoDevice.h"
#include "vtk_glad.h"

#include <algorithm>
#include <chrono>

vtkStandardNewMacro(vtkVulkanEncoder);

//------------------------------------------------------------------------------
vtkVulkanEncoder::vtkVulkanEncoder()
{
  this->Codec = VTKVideoCodecType::VTKVC_H264;
}

//------------------------------------------------------------------------------
vtkVulkanEncoder::~vtkVulkanEncoder()
{
  this->Internals.reset();
  this->Device.reset();
}

//------------------------------------------------------------------------------
void vtkVulkanEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Device: " << (this->Device ? "created" : "(none)") << '\n';
  os << indent
     << "Session: " << (this->Internals && this->Internals->IsReady() ? "ready" : "(none)") << '\n';
}

//------------------------------------------------------------------------------
vtkIdType vtkVulkanEncoder::GetLastEncodeTimeNS() const noexcept
{
  return this->Internals
    ? std::chrono::duration_cast<std::chrono::nanoseconds>(this->Internals->dtEncode).count()
    : 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkVulkanEncoder::GetLastScaleTimeNS() const noexcept
{
  return 0;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoder::SupportsCodec(VTKVideoCodecType codec) const noexcept
{
  return codec == VTKVideoCodecType::VTKVC_H264;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoder::CheckAvailability() noexcept
{
  return vtkVulkanVideoDevice::CheckAvailability();
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoder::InitializeInternal()
{
  if (this->Codec != VTKVideoCodecType::VTKVC_H264)
  {
    vtkLogF(ERROR, "Vulkan encoder only supports H.264. Got codec %s.",
      vtkVideoCodecTypeUtilities::ToString(this->Codec));
    return false;
  }
  auto* glWindow = vtkOpenGLRenderWindow::SafeDownCast(this->GetGraphicsContext());
  if (!glWindow)
  {
    vtkLog(ERROR, "GraphicsContext not set or not an OpenGL render window");
    return false;
  }
  if (this->Device && this->Device->IsReady())
  {
    return true;
  }
  // Exported memory is only meaningful to the GPU that owns it, so the Vulkan device must be
  // the one the GL context runs on (not merely any GPU that can encode).
  unsigned char glDeviceUUID[16];
  if (!vtkVulkanGLInterop::GetDeviceUUID(glWindow, glDeviceUUID))
  {
    vtkLog(ERROR, "The OpenGL context does not support GL_EXT_memory_object.");
    return false;
  }
  this->Device = std::make_unique<vtkVulkanVideoDevice>();
  if (!this->Device->CreateInstance() || !this->Device->SelectPhysicalDevice(glDeviceUUID) ||
    !this->Device->CreateDevice())
  {
    vtkLogF(ERROR,
      "The GPU driving the OpenGL context (%s) has no Vulkan H.264 video encode support. On "
      "multi-GPU systems the window must run on the encode-capable GPU (e.g. PRIME render "
      "offload).",
      reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    this->Device.reset();
    return false;
  }
  this->Internals = std::make_unique<vtkVulkanEncoderInternals>(*this->Device);
  return true;
}

//------------------------------------------------------------------------------
void vtkVulkanEncoder::ShutdownInternal()
{
  if (this->GLInterop)
  {
    auto* glWindow = vtkOpenGLRenderWindow::SafeDownCast(this->GetGraphicsContext());
    this->GLInterop->Destroy(glWindow);
    this->GLInterop.reset();
  }
  this->Internals.reset();
  this->Device.reset();
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoder::SetupEncoderFrame(int width, int height)
{
  vtkLogF(TRACE, "%s, %dx%d", __func__, width, height);
  if (!this->Internals)
  {
    return false;
  }
  using RC = vtkVulkanEncoderInternals::RateControlMode;
  vtkVulkanEncoderInternals::Config config;
  config.Width = width;
  config.Height = height;
  config.GopSize = this->KeyFramesOnly ? 1 : this->GroupOfPicturesSize;
  config.FrameRateNumerator = static_cast<uint32_t>(std::max(1, this->TimeBaseEnd));
  config.FrameRateDenominator = static_cast<uint32_t>(std::max(1, this->TimeBaseStart));
  switch (this->BitRateControlMode)
  { 
    case BRCType::VBR:
      config.RateControl = RC::VariableBitrate;
      break;
    case BRCType::CQP:
    case BRCType::QP:
      config.RateControl = RC::ConstantQp;
      break;
    case BRCType::CBR:
    default:
      config.RateControl = RC::ConstantBitrate;
      break;
  }
  config.AverageBitrate = this->BitRate;
  config.MaxBitrate = this->MaxBitRate;
  config.Qp = static_cast<int>(this->QuantizationParameter);
  config.MinQp = static_cast<int>(this->MinQuantizationParameter);
  config.MaxQp = static_cast<int>(this->MaxQuantizationParameter);
  config.LowDelay = this->LowDelayMode;
  config.FlipInput = true; // GL framebuffers are bottom-up.
  if (this->MaximumBFrames > 0)
  {
    vtkLog(WARNING, "Vulkan encoder does not emit B-frames yet; ignoring MaximumBFrames.");
  }
  if (!this->Internals->Setup(config))
  {
    return false;
  }
  auto* glWindow = vtkOpenGLRenderWindow::SafeDownCast(this->GetGraphicsContext());
  if (!glWindow)
  {
    vtkLog(ERROR, "GraphicsContext not set or not an OpenGL render window");
    return false;
  }
  this->GLInterop = std::make_unique<vtkVulkanGLInterop>();
  if (!this->GLInterop->Import(
        glWindow, *this->Internals, this->Internals->GetAlignedWidth(),
        this->Internals->GetAlignedHeight()))
  {
    vtkLog(ERROR, "Failed to import the Vulkan input image into OpenGL.");
    this->GLInterop.reset();
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkVulkanEncoder::TearDownEncoderFrame()
{
  if (this->GLInterop)
  {
    auto* glWindow = vtkOpenGLRenderWindow::SafeDownCast(this->GetGraphicsContext());
    this->GLInterop->Destroy(glWindow);
    this->GLInterop.reset();
  }
  if (this->Internals)
  {
    this->Internals->TearDown();
  }
}

//------------------------------------------------------------------------------
namespace
{
vtkSmartPointer<vtkCompressedVideoPacket> MakePacket(
  vtkVulkanEncoderInternals::EncodedFrame& encoded, const std::string& codecName, int width,
  int height)
{
  auto packet = vtk::TakeSmartPointer(vtkCompressedVideoPacket::New());
  packet->SetIsKeyFrame(encoded.IsKeyFrame);
  packet->SetCodecLongName(codecName.c_str());
  packet->SetDisplayWidth(width);
  packet->SetDisplayHeight(height);
  packet->SetCodedWidth(width);
  packet->SetCodedHeight(height);
  packet->SetPresentationTS(encoded.PresentationTS);
  packet->CopyData(encoded.Bitstream.data(), static_cast<int>(encoded.Bitstream.size()));
  vtkLogF(TRACE, "%lld|%s|%d bytes", packet->GetPresentationTS(),
    packet->GetIsKeyFrame() ? "key" : "delta", packet->GetSize());
  return packet;
}
} // anonymous namespace

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVulkanEncoder::EncodeInternal(vtkSmartPointer<vtkRawVideoFrame> frame)
{
  if (frame == nullptr || !this->Internals || !this->Internals->IsReady())
  {
    return {};
  }
  // Pixel data in `frame` is ignored -- it only carries width/height/key-frame flags for the
  // base class. The encoder blits the OpenGL render window's colour buffer into a shared
  // Vulkan image and converts it to NV12 on the Vulkan side; see vtkVulkanGLInterop.
  auto* glWindow = vtkOpenGLRenderWindow::SafeDownCast(this->GetGraphicsContext());
  if (!glWindow || !this->GLInterop || !this->GLInterop->IsReady())
  {
    vtkLog(ERROR, "GraphicsContext not set, not an OpenGL render window, or not yet imported");
    return { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} };
  }
  if (!this->GLInterop->CaptureFrame(glWindow, this->Width, this->Height))
  {
    return { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} };
  }
  vtkVulkanEncoderInternals::EncodedFrame encoded;
  const bool forceKey = this->ForceIFrame || this->KeyFramesOnly || frame->GetIsKeyFrame();
  if (!this->Internals->EncodeFrame(forceKey, encoded))
  {
    return { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} };
  }
  return { VTKVideoProcessingStatusType::VTKVPStatus_Success,
    { ::MakePacket(encoded, this->Internals->GetCodecName(), this->Width, this->Height) } };
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkVulkanEncoder::SendEOS()
{
  // Frames are encoded synchronously; nothing is buffered.
  return { VTKVideoProcessingStatusType::VTKVPStatus_Success, {} };
}

//------------------------------------------------------------------------------
// TEMP (VTK 9.6): self-register this backend with vtkEncoderFactory so it can be
// selected by preferences. Delete this block for VTK 9.7 and instead return these
// attributes from vtkVulkanEncoder::CreateOverrideAttributes().
#include "vtkEncoderFactory.h"
namespace
{
vtkVideoEncoder* CreateVulkanEncoder()
{
  return vtkVulkanEncoder::New();
}

struct vtkVulkanEncoderRegistrar
{
  vtkVulkanEncoderRegistrar()
  {
    vtkEncoderFactory::BackendDescriptor d;
    d.SubclassName = "vtkVulkanEncoder";
    d.Create = &CreateVulkanEncoder;
    d.Available = &vtkVulkanEncoder::CheckAvailability;
    d.Hardware = true;
    d.Codecs = { VTKVideoCodecType::VTKVC_H264 };
    d.Attributes = { { "Platform", "Linux" }, { "Hardware", "true" } };
    vtkEncoderFactory::RegisterBackend(d);
  }
};
// Runs when the vtkStreamingVulkanEncode library is loaded (e.g. on `import vtk_streaming`).
const vtkVulkanEncoderRegistrar sVulkanEncoderRegistrar;
} // anonymous namespace
