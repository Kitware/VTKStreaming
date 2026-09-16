// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "vtkVulkanEncoderInternals.h"

#include "vtkLogger.h"
#include "vtkVulkanRGBAToNV12CS.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#define VTK_VK_CHECK(call)                                                                         \
  do                                                                                               \
  {                                                                                                \
    const VkResult vtkVkResult = (call);                                                           \
    if (vtkVkResult != VK_SUCCESS)                                                                 \
    {                                                                                              \
      vtkLogF(ERROR, "%s failed (%d)", #call, static_cast<int>(vtkVkResult));                      \
      return false;                                                                                \
    }                                                                                              \
  } while (0)

namespace
{
constexpr uint32_t MacroblockSize = 16;
constexpr uint32_t DpbSlotCount = 2;
constexpr uint32_t Log2MaxFrameNum = 8;
constexpr uint32_t Log2MaxPocLsb = 12;
// Must match the layout(local_size_*) and layout(push_constant) of vtkVulkanRGBAToNV12CS.comp.
constexpr uint32_t ConvertWorkgroupSize = 8;
struct ConvertPushConstants
{
  uint32_t Width;
  uint32_t Height;
  uint32_t FlipY;
};

template <typename T>
T AlignUp(T value, T alignment)
{
  return (value + alignment - 1) / alignment * alignment;
}

StdVideoH264LevelIdc ChooseLevel(uint32_t macroblocks, uint32_t fps, StdVideoH264LevelIdc maxLevel)
{
  struct Level
  {
    StdVideoH264LevelIdc Idc;
    uint32_t MaxFrameMbs;
    uint32_t MaxMbsPerSecond;
  };
  static const Level levels[] = {
    { STD_VIDEO_H264_LEVEL_IDC_3_1, 3600, 108000 },
    { STD_VIDEO_H264_LEVEL_IDC_4_1, 8192, 245760 },
    { STD_VIDEO_H264_LEVEL_IDC_5_1, 36864, 983040 },
    { STD_VIDEO_H264_LEVEL_IDC_5_2, 36864, 2073600 },
    { STD_VIDEO_H264_LEVEL_IDC_6_2, 139264, 16711680 },
  };
  for (const auto& level : levels)
  {
    if (macroblocks <= level.MaxFrameMbs && macroblocks * fps <= level.MaxMbsPerSecond)
    {
      return std::min(level.Idc, maxLevel);
    }
  }
  return maxLevel;
}

VkImageMemoryBarrier2 MakeImageBarrier(VkImage image, uint32_t layers, VkImageLayout oldLayout,
  VkImageLayout newLayout, VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
  VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess)
{
  VkImageMemoryBarrier2 barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
  barrier.srcStageMask = srcStage;
  barrier.srcAccessMask = srcAccess;
  barrier.dstStageMask = dstStage;
  barrier.dstAccessMask = dstAccess;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layers };
  return barrier;
}

void CmdImageBarrier(
  const vtkVulkanFunctions& vk, VkCommandBuffer cmd, const VkImageMemoryBarrier2& barrier)
{
  VkDependencyInfo dependency = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
  dependency.imageMemoryBarrierCount = 1;
  dependency.pImageMemoryBarriers = &barrier;
  vk.vkCmdPipelineBarrier2(cmd, &dependency);
}

void CmdBufferBarrier(const vtkVulkanFunctions& vk, VkCommandBuffer cmd, VkBuffer buffer,
  VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage,
  VkAccessFlags2 dstAccess)
{
  VkBufferMemoryBarrier2 barrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
  barrier.srcStageMask = srcStage;
  barrier.srcAccessMask = srcAccess;
  barrier.dstStageMask = dstStage;
  barrier.dstAccessMask = dstAccess;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = buffer;
  barrier.size = VK_WHOLE_SIZE;
  VkDependencyInfo dependency = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
  dependency.bufferMemoryBarrierCount = 1;
  dependency.pBufferMemoryBarriers = &barrier;
  vk.vkCmdPipelineBarrier2(cmd, &dependency);
}
} // namespace

//------------------------------------------------------------------------------
vtkVulkanEncoderInternals::vtkVulkanEncoderInternals(vtkVulkanVideoDevice& device)
  : Device(device)
{
}

//------------------------------------------------------------------------------
vtkVulkanEncoderInternals::~vtkVulkanEncoderInternals()
{
  this->TearDown();
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::Setup(const Config& config)
{
  if (!this->Device.IsReady())
  {
    vtkLog(ERROR, "Vulkan device is not ready.");
    return false;
  }
  this->TearDown();
  this->Settings = config;

  // Mesa's ANV H.264 encoder emits rbsp trailing bits between the slice header and the slice
  // data before 26.0; the output is repaired in FixSliceHeaderPadding().
  const auto& driver = this->Device.DriverProperties;
  this->NeedsSliceHeaderPaddingFix = driver.driverID == VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA &&
    VK_VERSION_MAJOR(this->Device.Properties.driverVersion) < 26;
  if (this->NeedsSliceHeaderPaddingFix)
  {
    vtkLogF(INFO, "%s: stripping slice header padding from the encoded bitstream (Mesa < 26.0)",
      driver.driverInfo);
  }

  this->H264Profile = { VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR };
  this->H264Profile.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
  this->Profile = { VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR, /*pNext=*/&this->H264Profile };
  this->Profile.videoCodecOperation = VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR;
  this->Profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
  this->Profile.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
  this->Profile.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
  this->ProfileList = { VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR };
  this->ProfileList.profileCount = 1;
  this->ProfileList.pProfiles = &this->Profile;

  bool ok = this->QueryCapabilities() && this->QueryFormats() && this->CreateSession() &&
    this->CreateSessionParameters() && this->EmitParameterSets() &&
    this->CreateNV12SourceImage() && this->CreateInputImage() &&
    (this->DirectConvert || this->CreatePlaneImages()) && this->CreateConvertPipeline() &&
    this->CreateDpbImage() && this->CreateBuffers() && this->CreateQueryPool() &&
    this->CreateCommandResources() && this->TransitionDpbToEncodeLayout() &&
    this->CreateSemaphore(this->ReadySemaphore, /*exportable=*/true) &&
    this->CreateSemaphore(this->FreeSemaphore, /*exportable=*/true) &&
    this->CreateSemaphore(this->ConvertDoneSemaphore, /*exportable=*/false) &&
    this->InitializeInteropImageLayout();
  if (!ok)
  {
    this->TearDown();
  }
  return ok;
}

//------------------------------------------------------------------------------
void vtkVulkanEncoderInternals::TearDown()
{
  auto& vk = this->Device.Functions;
  VkDevice device = this->Device.Device;
  if (device == VK_NULL_HANDLE)
  {
    return;
  }
  vk.vkDeviceWaitIdle(device);
  for (VkSemaphore* semaphore :
    { &this->ReadySemaphore, &this->FreeSemaphore, &this->ConvertDoneSemaphore })
  {
    if (*semaphore != VK_NULL_HANDLE)
    {
      vk.vkDestroySemaphore(device, *semaphore, nullptr);
      *semaphore = VK_NULL_HANDLE;
    }
  }
  if (this->Fence != VK_NULL_HANDLE)
  {
    vk.vkDestroyFence(device, this->Fence, nullptr);
    this->Fence = VK_NULL_HANDLE;
  }
  for (VkCommandPool* pool : { &this->EncodeCommandPool, &this->ComputeCommandPool })
  {
    if (*pool != VK_NULL_HANDLE)
    {
      vk.vkDestroyCommandPool(device, *pool, nullptr);
      *pool = VK_NULL_HANDLE;
    }
  }
  this->EncodeCommandBuffer = this->ComputeCommandBuffer = VK_NULL_HANDLE;
  if (this->QueryPool != VK_NULL_HANDLE)
  {
    vk.vkDestroyQueryPool(device, this->QueryPool, nullptr);
    this->QueryPool = VK_NULL_HANDLE;
  }
  this->DestroyBuffer(this->BitstreamBuffer);
  this->DestroyConvertPipeline();
  for (VkImageView& view : this->SourcePlaneViews)
  {
    if (view != VK_NULL_HANDLE)
    {
      vk.vkDestroyImageView(device, view, nullptr);
      view = VK_NULL_HANDLE;
    }
  }
  this->DestroyImage(this->SourceImage);
  this->DestroyImage(this->InputImage);
  for (Image& plane : this->PlaneImages)
  {
    this->DestroyImage(plane);
  }
  this->DestroyImage(this->DpbImage);
  if (this->SessionParameters != VK_NULL_HANDLE)
  {
    vk.vkDestroyVideoSessionParametersKHR(device, this->SessionParameters, nullptr);
    this->SessionParameters = VK_NULL_HANDLE;
  }
  if (this->Session != VK_NULL_HANDLE)
  {
    vk.vkDestroyVideoSessionKHR(device, this->Session, nullptr);
    this->Session = VK_NULL_HANDLE;
  }
  for (VkDeviceMemory memory : this->SessionMemory)
  {
    vk.vkFreeMemory(device, memory, nullptr);
  }
  this->SessionMemory.clear();
  this->ParameterSetBytes.clear();

  this->RateControlApplied = false;
  this->DirectConvert = false;
  this->SourceImageCreateFlags = 0;
  this->FrameCount = 0;
  this->FrameIndexInGop = 0;
  this->FrameNum = 0;
  this->IdrPicId = 0;
  this->CurrentSlot = 0;
  this->SlotActive[0] = this->SlotActive[1] = false;
  this->HaveReference = false;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::QueryCapabilities()
{
  auto& vk = this->Device.Functions;
  this->H264Capabilities = { VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_CAPABILITIES_KHR };
  this->EncodeCapabilities = { VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR,
    &this->H264Capabilities };
  this->Capabilities = { VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR, &this->EncodeCapabilities };
  VTK_VK_CHECK(vk.vkGetPhysicalDeviceVideoCapabilitiesKHR(
    this->Device.PhysicalDevice, &this->Profile, &this->Capabilities));

  const auto& caps = this->Capabilities;
  if (caps.maxDpbSlots < DpbSlotCount || caps.maxActiveReferencePictures < 1)
  {
    vtkLogF(ERROR, "H.264 encoder exposes %u DPB slots / %u active references; need %u / 1",
      caps.maxDpbSlots, caps.maxActiveReferencePictures, DpbSlotCount);
    return false;
  }
  const uint32_t alignX = std::max(MacroblockSize, caps.pictureAccessGranularity.width);
  const uint32_t alignY = std::max(MacroblockSize, caps.pictureAccessGranularity.height);
  this->AlignedWidth = AlignUp(static_cast<uint32_t>(this->Settings.Width), alignX);
  this->AlignedHeight = AlignUp(static_cast<uint32_t>(this->Settings.Height), alignY);
  if (this->AlignedWidth < caps.minCodedExtent.width ||
    this->AlignedHeight < caps.minCodedExtent.height ||
    this->AlignedWidth > caps.maxCodedExtent.width ||
    this->AlignedHeight > caps.maxCodedExtent.height)
  {
    vtkLogF(ERROR, "%ux%u is outside the supported coded extent [%ux%u, %ux%u]", this->AlignedWidth,
      this->AlignedHeight, caps.minCodedExtent.width, caps.minCodedExtent.height,
      caps.maxCodedExtent.width, caps.maxCodedExtent.height);
    return false;
  }
  const VkVideoEncodeFeedbackFlagsKHR neededFeedback =
    VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BUFFER_OFFSET_BIT_KHR |
    VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BYTES_WRITTEN_BIT_KHR;
  if ((this->EncodeCapabilities.supportedEncodeFeedbackFlags & neededFeedback) != neededFeedback)
  {
    vtkLog(ERROR, "Encoder does not report bitstream offset/size feedback.");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::QueryFormats()
{
  auto& vk = this->Device.Functions;
  // Finds the optimal-tiling 2D NV12 entry among the formats usable as `usage` under the H.264
  // profile; `out` receives the driver's create flags / usage for it.
  auto pickFormat = [&](VkImageUsageFlags usage, VkVideoFormatPropertiesKHR& out) -> bool
  {
    VkPhysicalDeviceVideoFormatInfoKHR info = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_FORMAT_INFO_KHR, &this->ProfileList
    };
    info.imageUsage = usage;
    uint32_t count = 0;
    if (vk.vkGetPhysicalDeviceVideoFormatPropertiesKHR(
          this->Device.PhysicalDevice, &info, &count, nullptr) != VK_SUCCESS)
    {
      return false;
    }
    std::vector<VkVideoFormatPropertiesKHR> props(
      count, { VK_STRUCTURE_TYPE_VIDEO_FORMAT_PROPERTIES_KHR });
    if (vk.vkGetPhysicalDeviceVideoFormatPropertiesKHR(
          this->Device.PhysicalDevice, &info, &count, props.data()) != VK_SUCCESS)
    {
      return false;
    }
    for (const auto& p : props)
    {
      if (p.format == VK_FORMAT_G8_B8R8_2PLANE_420_UNORM && p.imageType == VK_IMAGE_TYPE_2D &&
        p.imageTiling == VK_IMAGE_TILING_OPTIMAL)
      {
        out = p;
        return true;
      }
    }
    return false;
  };

  VkVideoFormatPropertiesKHR sourceProps, dpbProps;
  const VkImageUsageFlags copyUsage =
    VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if (!pickFormat(copyUsage, sourceProps))
  {
    vtkLogF(ERROR, "No NV12 image format for video usage 0x%x", copyUsage);
    return false;
  }
  if (!pickFormat(VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR, dpbProps))
  {
    vtkLogF(ERROR, "No NV12 image format for video usage 0x%x",
      VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR);
    return false;
  }
  this->SourceFormat = sourceProps.format;
  this->DpbFormat = dpbProps.format;

  // Can the conversion shader store straight into the encode source image? That needs
  // STORAGE usage on it plus MUTABLE_FORMAT so R8/RG8 views of its planes can be made. Both
  // the video format query (which some drivers merely echo the requested usage back from) and
  // vkGetPhysicalDeviceImageFormatProperties2 must agree, as image creation requires. As of
  // Mesa 25.0 / NVIDIA 615 neither does, so the intermediate-image path below is the one in use.
  this->DirectConvert = false;
  this->SourceImageCreateFlags = 0;
  const VkImageUsageFlags directUsage =
    VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR | VK_IMAGE_USAGE_STORAGE_BIT;
  VkVideoFormatPropertiesKHR directProps;
  if (pickFormat(directUsage, directProps) &&
    (directProps.imageUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) &&
    (directProps.imageCreateFlags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT))
  {
    const VkImageCreateFlags flags = directProps.imageCreateFlags &
      (VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT);
    VkPhysicalDeviceImageFormatInfo2 info = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, &this->ProfileList
    };
    info.format = this->SourceFormat;
    info.type = VK_IMAGE_TYPE_2D;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = directUsage;
    info.flags = flags;
    VkImageFormatProperties2 props = { VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2 };
    if (vk.vkGetPhysicalDeviceImageFormatProperties2(this->Device.PhysicalDevice, &info, &props) ==
      VK_SUCCESS)
    {
      this->DirectConvert = true;
      this->SourceImageCreateFlags = flags;
    }
  }
  vtkLogF(INFO, "RGBA->NV12 conversion writes %s",
    this->DirectConvert ? "the encode source image directly"
                        : "intermediate images that are copied into the encode source image");
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateSession()
{
  auto& vk = this->Device.Functions;
  VkDevice device = this->Device.Device;

  VkExtensionProperties stdHeader = {};
  std::strncpy(stdHeader.extensionName, VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_EXTENSION_NAME,
    VK_MAX_EXTENSION_NAME_SIZE - 1);
  stdHeader.specVersion = std::min<uint32_t>(VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_SPEC_VERSION,
    this->Capabilities.stdHeaderVersion.specVersion);

  VkVideoSessionCreateInfoKHR info = { VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR };
  info.queueFamilyIndex = this->Device.EncodeQueueFamily;
  info.pVideoProfile = &this->Profile;
  info.pictureFormat = this->SourceFormat;
  info.maxCodedExtent = { this->AlignedWidth, this->AlignedHeight };
  info.referencePictureFormat = this->DpbFormat;
  info.maxDpbSlots = DpbSlotCount;
  info.maxActiveReferencePictures = 1;
  info.pStdHeaderVersion = &stdHeader;
  VTK_VK_CHECK(vk.vkCreateVideoSessionKHR(device, &info, nullptr, &this->Session));

  uint32_t count = 0;
  VTK_VK_CHECK(vk.vkGetVideoSessionMemoryRequirementsKHR(device, this->Session, &count, nullptr));
  std::vector<VkVideoSessionMemoryRequirementsKHR> requirements(
    count, { VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR });
  VTK_VK_CHECK(
    vk.vkGetVideoSessionMemoryRequirementsKHR(device, this->Session, &count, requirements.data()));

  std::vector<VkBindVideoSessionMemoryInfoKHR> binds;
  for (const auto& req : requirements)
  {
    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (!this->AllocateMemory(
          req.memoryRequirements, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory))
    {
      return false;
    }
    this->SessionMemory.push_back(memory);
    VkBindVideoSessionMemoryInfoKHR bind = { VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR };
    bind.memoryBindIndex = req.memoryBindIndex;
    bind.memory = memory;
    bind.memoryOffset = 0;
    bind.memorySize = req.memoryRequirements.size;
    binds.push_back(bind);
  }
  VTK_VK_CHECK(vk.vkBindVideoSessionMemoryKHR(
    device, this->Session, static_cast<uint32_t>(binds.size()), binds.data()));
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateSessionParameters()
{
  auto& vk = this->Device.Functions;
  const uint32_t widthInMbs = this->AlignedWidth / MacroblockSize;
  const uint32_t heightInMbs = this->AlignedHeight / MacroblockSize;
  const uint32_t fps =
    std::max(1u, this->Settings.FrameRateNumerator / this->Settings.FrameRateDenominator);

  this->Sps = {};
  this->Sps.flags.direct_8x8_inference_flag = 1;
  this->Sps.flags.frame_mbs_only_flag = 1;
  this->Sps.profile_idc = this->H264Profile.stdProfileIdc;
  this->Sps.level_idc =
    ChooseLevel(widthInMbs * heightInMbs, fps, this->H264Capabilities.maxLevelIdc);
  this->Sps.chroma_format_idc = STD_VIDEO_H264_CHROMA_FORMAT_IDC_420;
  this->Sps.log2_max_frame_num_minus4 = Log2MaxFrameNum - 4;
  this->Sps.pic_order_cnt_type = STD_VIDEO_H264_POC_TYPE_0;
  this->Sps.log2_max_pic_order_cnt_lsb_minus4 = Log2MaxPocLsb - 4;
  this->Sps.max_num_ref_frames = 1;
  this->Sps.pic_width_in_mbs_minus1 = widthInMbs - 1;
  this->Sps.pic_height_in_map_units_minus1 = heightInMbs - 1;
  const uint32_t width = static_cast<uint32_t>(this->Settings.Width);
  const uint32_t height = static_cast<uint32_t>(this->Settings.Height);
  if (this->AlignedWidth != width || this->AlignedHeight != height)
  {
    // Crop units are 2 luma samples for 4:2:0 frame pictures.
    this->Sps.flags.frame_cropping_flag = 1;
    this->Sps.frame_crop_right_offset = (this->AlignedWidth - width) / 2;
    this->Sps.frame_crop_bottom_offset = (this->AlignedHeight - height) / 2;
  }

  this->Pps = {};
  this->Pps.flags.entropy_coding_mode_flag =
    (this->H264Capabilities.stdSyntaxFlags &
      VK_VIDEO_ENCODE_H264_STD_ENTROPY_CODING_MODE_FLAG_SET_BIT_KHR) != 0;
  this->Pps.flags.deblocking_filter_control_present_flag = 1;
  this->Pps.weighted_bipred_idc = STD_VIDEO_H264_WEIGHTED_BIPRED_IDC_DEFAULT;

  VkVideoEncodeH264SessionParametersAddInfoKHR add = {
    VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR
  };
  add.stdSPSCount = 1;
  add.pStdSPSs = &this->Sps;
  add.stdPPSCount = 1;
  add.pStdPPSs = &this->Pps;
  VkVideoEncodeH264SessionParametersCreateInfoKHR h264 = {
    VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR
  };
  h264.maxStdSPSCount = 1;
  h264.maxStdPPSCount = 1;
  h264.pParametersAddInfo = &add;
  VkVideoSessionParametersCreateInfoKHR info = {
    VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR, &h264
  };
  info.videoSession = this->Session;
  VTK_VK_CHECK(vk.vkCreateVideoSessionParametersKHR(
    this->Device.Device, &info, nullptr, &this->SessionParameters));
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::EmitParameterSets()
{
  auto& vk = this->Device.Functions;
  VkVideoEncodeH264SessionParametersGetInfoKHR h264Get = {
    VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_GET_INFO_KHR
  };
  h264Get.writeStdSPS = VK_TRUE;
  h264Get.writeStdPPS = VK_TRUE;
  VkVideoEncodeSessionParametersGetInfoKHR get = {
    VK_STRUCTURE_TYPE_VIDEO_ENCODE_SESSION_PARAMETERS_GET_INFO_KHR, &h264Get
  };
  get.videoSessionParameters = this->SessionParameters;
  VkVideoEncodeH264SessionParametersFeedbackInfoKHR h264Feedback = {
    VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_FEEDBACK_INFO_KHR
  };
  VkVideoEncodeSessionParametersFeedbackInfoKHR feedback = {
    VK_STRUCTURE_TYPE_VIDEO_ENCODE_SESSION_PARAMETERS_FEEDBACK_INFO_KHR, &h264Feedback
  };
  size_t size = 0;
  VTK_VK_CHECK(
    vk.vkGetEncodedVideoSessionParametersKHR(this->Device.Device, &get, &feedback, &size, nullptr));
  this->ParameterSetBytes.resize(size);
  VTK_VK_CHECK(vk.vkGetEncodedVideoSessionParametersKHR(
    this->Device.Device, &get, &feedback, &size, this->ParameterSetBytes.data()));
  this->ParameterSetBytes.resize(size);
  if (this->ParameterSetBytes.empty())
  {
    return false;
  }
  // Build the WebCodecs/RFC-6381 codec string from the SPS: avc1.PPCCLL, where PP is
  // profile_idc, CC the constraint_set flags byte and LL level_idc. The parameter sets are
  // Annex-B, so walk the start codes looking for NAL type 7. Bare "avc1" is rejected by
  // VideoDecoder.isConfigSupported().
  this->CodecName.clear();
  const auto& bytes = this->ParameterSetBytes;
  for (std::size_t i = 0; i + 3 < bytes.size(); ++i)
  {
    if (bytes[i] != 0 || bytes[i + 1] != 0 || bytes[i + 2] != 1)
    {
      continue;
    }
    const std::size_t nal = i + 3;
    if ((bytes[nal] & 0x1F) == 7 && nal + 3 < bytes.size())
    {
      char buf[16];
      std::snprintf(buf, sizeof(buf), "avc1.%02X%02X%02X", bytes[nal + 1], bytes[nal + 2],
        bytes[nal + 3]);
      this->CodecName = buf;
      break;
    }
  }
  if (this->CodecName.empty())
  {
    vtkLog(ERROR, "No SPS found in the encoded H.264 parameter sets.");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::AllocateMemory(const VkMemoryRequirements& reqs,
  VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred, VkDeviceMemory& memory)
{
  uint32_t typeIndex = 0;
  if (!this->Device.FindMemoryType(reqs.memoryTypeBits, required, preferred, typeIndex))
  {
    vtkLogF(ERROR, "No memory type for bits 0x%x with flags 0x%x", reqs.memoryTypeBits, required);
    return false;
  }
  VkMemoryAllocateInfo info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
  info.allocationSize = reqs.size;
  info.memoryTypeIndex = typeIndex;
  VTK_VK_CHECK(
    this->Device.Functions.vkAllocateMemory(this->Device.Device, &info, nullptr, &memory));
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::AllocateExportableMemory(
  const VkMemoryRequirements& reqs, VkImage dedicatedImage, VkDeviceMemory& memory)
{
  uint32_t typeIndex = 0;
  if (!this->Device.FindMemoryType(
        reqs.memoryTypeBits, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, typeIndex))
  {
    vtkLogF(ERROR, "No memory type for bits 0x%x", reqs.memoryTypeBits);
    return false;
  }
  VkMemoryDedicatedAllocateInfo dedicated = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
  dedicated.image = dedicatedImage;
  VkExportMemoryAllocateInfo exportInfo = { VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
    dedicatedImage != VK_NULL_HANDLE ? static_cast<const void*>(&dedicated) : nullptr };
  exportInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
  VkMemoryAllocateInfo info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &exportInfo };
  info.allocationSize = reqs.size;
  info.memoryTypeIndex = typeIndex;
  VTK_VK_CHECK(
    this->Device.Functions.vkAllocateMemory(this->Device.Device, &info, nullptr, &memory));
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateSemaphore(VkSemaphore& semaphore, bool exportable)
{
  VkExportSemaphoreCreateInfo exportInfo = { VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO };
  exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
  VkSemaphoreCreateInfo info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    exportable ? &exportInfo : nullptr };
  VTK_VK_CHECK(
    this->Device.Functions.vkCreateSemaphore(this->Device.Device, &info, nullptr, &semaphore));
  return true;
}

//------------------------------------------------------------------------------
int vtkVulkanEncoderInternals::ExportSemaphoreFd(VkSemaphore semaphore) const
{
  if (semaphore == VK_NULL_HANDLE)
  {
    return -1;
  }
  VkSemaphoreGetFdInfoKHR info = { VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR };
  info.semaphore = semaphore;
  info.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
  int fd = -1;
  if (this->Device.Functions.vkGetSemaphoreFdKHR(this->Device.Device, &info, &fd) != VK_SUCCESS)
  {
    vtkLog(ERROR, "vkGetSemaphoreFdKHR failed.");
    return -1;
  }
  return fd;
}

//------------------------------------------------------------------------------
int vtkVulkanEncoderInternals::ExportReadySemaphoreFd() const
{
  return this->ExportSemaphoreFd(this->ReadySemaphore);
}

//------------------------------------------------------------------------------
int vtkVulkanEncoderInternals::ExportFreeSemaphoreFd() const
{
  return this->ExportSemaphoreFd(this->FreeSemaphore);
}

//------------------------------------------------------------------------------
int vtkVulkanEncoderInternals::ExportInputMemoryFd() const
{
  VkDeviceMemory memory = this->InputImage.Memory;
  if (memory == VK_NULL_HANDLE)
  {
    return -1;
  }
  VkMemoryGetFdInfoKHR info = { VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR };
  info.memory = memory;
  info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
  int fd = -1;
  if (this->Device.Functions.vkGetMemoryFdKHR(this->Device.Device, &info, &fd) != VK_SUCCESS)
  {
    vtkLog(ERROR, "vkGetMemoryFdKHR failed.");
    return -1;
  }
  return fd;
}

//------------------------------------------------------------------------------
VkDeviceSize vtkVulkanEncoderInternals::GetInputMemorySize() const
{
  return this->InputImage.MemorySize;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateImage(VkImageUsageFlags usage, VkFormat format,
  VkExtent2D extent, uint32_t layers, Image& image, bool exportable, bool sharedWithComputeQueue,
  VkImageCreateFlags flags)
{
  auto& vk = this->Device.Functions;
  VkDevice device = this->Device.Device;
  const uint32_t queueFamilies[2] = { this->Device.ComputeQueueFamily,
    this->Device.EncodeQueueFamily };

  VkExternalMemoryImageCreateInfo externalInfo = {
    VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO
  };
  externalInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
  // A profile list is only valid (and only needed) on images with a video usage.
  const bool video =
    (usage & (VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR | VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR)) !=
    0;

  VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    exportable ? static_cast<const void*>(&externalInfo)
      : video  ? static_cast<const void*>(&this->ProfileList)
               : nullptr };
  info.flags = flags;
  info.imageType = VK_IMAGE_TYPE_2D;
  info.format = format;
  info.extent = { extent.width, extent.height, 1 };
  info.mipLevels = 1;
  info.arrayLayers = layers;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  info.usage = usage;
  info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (sharedWithComputeQueue && queueFamilies[0] != queueFamilies[1])
  {
    info.sharingMode = VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount = 2;
    info.pQueueFamilyIndices = queueFamilies;
  }
  VTK_VK_CHECK(vk.vkCreateImage(device, &info, nullptr, &image.Handle));
  image.Layers = layers;

  VkImageMemoryRequirementsInfo2 reqInfo = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2 };
  reqInfo.image = image.Handle;
  VkMemoryRequirements2 reqs = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
  vk.vkGetImageMemoryRequirements2(device, &reqInfo, &reqs);
  image.MemorySize = reqs.memoryRequirements.size;
  const bool allocated = exportable
    ? this->AllocateExportableMemory(reqs.memoryRequirements, image.Handle, image.Memory)
    : this->AllocateMemory(
        reqs.memoryRequirements, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image.Memory);
  if (!allocated)
  {
    return false;
  }
  VkBindImageMemoryInfo bind = { VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO };
  bind.image = image.Handle;
  bind.memory = image.Memory;
  VTK_VK_CHECK(vk.vkBindImageMemory2(device, 1, &bind));
  return this->CreateImageView(image.Handle, VK_IMAGE_ASPECT_COLOR_BIT, format, layers, image.View);
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateImageView(
  VkImage image, VkImageAspectFlags aspect, VkFormat format, uint32_t layers, VkImageView& view)
{
  auto& vk = this->Device.Functions;
  // Plane views of the NV12 source image carry a plane format the video usage does not apply
  // to, so restrict them to what the conversion shader does with them.
  VkImageViewUsageCreateInfo usageInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
  usageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
  const bool plane = aspect != VK_IMAGE_ASPECT_COLOR_BIT;
  VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    plane ? &usageInfo : nullptr };
  viewInfo.image = image;
  viewInfo.viewType = layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange = { aspect, 0, 1, 0, layers };
  VTK_VK_CHECK(vk.vkCreateImageView(this->Device.Device, &viewInfo, nullptr, &view));
  return true;
}

//------------------------------------------------------------------------------
void vtkVulkanEncoderInternals::DestroyImage(Image& image)
{
  auto& vk = this->Device.Functions;
  VkDevice device = this->Device.Device;
  if (image.View != VK_NULL_HANDLE)
  {
    vk.vkDestroyImageView(device, image.View, nullptr);
  }
  if (image.Handle != VK_NULL_HANDLE)
  {
    vk.vkDestroyImage(device, image.Handle, nullptr);
  }
  if (image.Memory != VK_NULL_HANDLE)
  {
    vk.vkFreeMemory(device, image.Memory, nullptr);
  }
  image = {};
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateNV12SourceImage()
{
  const VkImageUsageFlags usage = VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR |
    (this->DirectConvert ? VK_IMAGE_USAGE_STORAGE_BIT : VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  if (!this->CreateImage(usage, this->SourceFormat, { this->AlignedWidth, this->AlignedHeight },
        1, this->SourceImage, /*exportable=*/false, /*sharedWithComputeQueue=*/true,
        this->SourceImageCreateFlags))
  {
    return false;
  }
  // The shader stores through single-plane views of the two NV12 planes.
  return !this->DirectConvert ||
    (this->CreateImageView(this->SourceImage.Handle, VK_IMAGE_ASPECT_PLANE_0_BIT,
       VK_FORMAT_R8_UNORM, 1, this->SourcePlaneViews[0]) &&
      this->CreateImageView(this->SourceImage.Handle, VK_IMAGE_ASPECT_PLANE_1_BIT,
        VK_FORMAT_R8G8_UNORM, 1, this->SourcePlaneViews[1]));
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateInputImage()
{
  // GL blits (or copies) into it; the conversion shader reads it as a storage image.
  const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
  return this->CreateImage(usage, VK_FORMAT_R8G8B8A8_UNORM,
    { this->AlignedWidth, this->AlignedHeight }, 1, this->InputImage, /*exportable=*/true);
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreatePlaneImages()
{
  const VkImageUsageFlags usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  return this->CreateImage(usage, VK_FORMAT_R8_UNORM, { this->AlignedWidth, this->AlignedHeight },
           1, this->PlaneImages[0]) &&
    this->CreateImage(usage, VK_FORMAT_R8G8_UNORM,
      { this->AlignedWidth / 2, this->AlignedHeight / 2 }, 1, this->PlaneImages[1]);
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateConvertPipeline()
{
  auto& vk = this->Device.Functions;
  VkDevice device = this->Device.Device;

  VkShaderModuleCreateInfo shaderInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
  shaderInfo.codeSize = sizeof(vtkVulkanRGBAToNV12CS);
  shaderInfo.pCode = vtkVulkanRGBAToNV12CS;
  VTK_VK_CHECK(vk.vkCreateShaderModule(device, &shaderInfo, nullptr, &this->ConvertShader));

  // binding 0: RGBA input, 1: luma, 2: chroma -- all storage images.
  VkDescriptorSetLayoutBinding bindings[3] = {};
  for (uint32_t i = 0; i < 3; ++i)
  {
    bindings[i].binding = i;
    bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[i].descriptorCount = 1;
    bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  }
  VkDescriptorSetLayoutCreateInfo setLayoutInfo = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
  };
  setLayoutInfo.bindingCount = 3;
  setLayoutInfo.pBindings = bindings;
  VTK_VK_CHECK(
    vk.vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &this->ConvertSetLayout));

  VkPushConstantRange pushRange = {};
  pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushRange.size = sizeof(ConvertPushConstants);
  VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &this->ConvertSetLayout;
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &pushRange;
  VTK_VK_CHECK(
    vk.vkCreatePipelineLayout(device, &layoutInfo, nullptr, &this->ConvertPipelineLayout));

  VkComputePipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
  pipelineInfo.stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
  pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipelineInfo.stage.module = this->ConvertShader;
  pipelineInfo.stage.pName = "main";
  pipelineInfo.layout = this->ConvertPipelineLayout;
  VTK_VK_CHECK(vk.vkCreateComputePipelines(
    device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->ConvertPipeline));

  VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 };
  VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
  poolInfo.maxSets = 1;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  VTK_VK_CHECK(vk.vkCreateDescriptorPool(device, &poolInfo, nullptr, &this->ConvertDescriptorPool));
  VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
  allocInfo.descriptorPool = this->ConvertDescriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &this->ConvertSetLayout;
  VTK_VK_CHECK(vk.vkAllocateDescriptorSets(device, &allocInfo, &this->ConvertDescriptorSet));

  // The images never change for the session's lifetime, so bind them once.
  const VkImageView targets[2] = {
    this->DirectConvert ? this->SourcePlaneViews[0] : this->PlaneImages[0].View,
    this->DirectConvert ? this->SourcePlaneViews[1] : this->PlaneImages[1].View,
  };
  VkDescriptorImageInfo imageInfos[3] = {};
  imageInfos[0].imageView = this->InputImage.View;
  imageInfos[1].imageView = targets[0];
  imageInfos[2].imageView = targets[1];
  VkWriteDescriptorSet writes[3] = {};
  for (uint32_t i = 0; i < 3; ++i)
  {
    imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    writes[i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    writes[i].dstSet = this->ConvertDescriptorSet;
    writes[i].dstBinding = i;
    writes[i].descriptorCount = 1;
    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[i].pImageInfo = &imageInfos[i];
  }
  vk.vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);
  return true;
}

//------------------------------------------------------------------------------
void vtkVulkanEncoderInternals::DestroyConvertPipeline()
{
  auto& vk = this->Device.Functions;
  VkDevice device = this->Device.Device;
  if (this->ConvertDescriptorPool != VK_NULL_HANDLE)
  {
    // Frees the set too.
    vk.vkDestroyDescriptorPool(device, this->ConvertDescriptorPool, nullptr);
    this->ConvertDescriptorPool = VK_NULL_HANDLE;
    this->ConvertDescriptorSet = VK_NULL_HANDLE;
  }
  if (this->ConvertPipeline != VK_NULL_HANDLE)
  {
    vk.vkDestroyPipeline(device, this->ConvertPipeline, nullptr);
    this->ConvertPipeline = VK_NULL_HANDLE;
  }
  if (this->ConvertPipelineLayout != VK_NULL_HANDLE)
  {
    vk.vkDestroyPipelineLayout(device, this->ConvertPipelineLayout, nullptr);
    this->ConvertPipelineLayout = VK_NULL_HANDLE;
  }
  if (this->ConvertSetLayout != VK_NULL_HANDLE)
  {
    vk.vkDestroyDescriptorSetLayout(device, this->ConvertSetLayout, nullptr);
    this->ConvertSetLayout = VK_NULL_HANDLE;
  }
  if (this->ConvertShader != VK_NULL_HANDLE)
  {
    vk.vkDestroyShaderModule(device, this->ConvertShader, nullptr);
    this->ConvertShader = VK_NULL_HANDLE;
  }
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateDpbImage()
{
  // One layer per slot works whether or not separate reference images are supported.
  return this->CreateImage(VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR, this->DpbFormat,
    { this->AlignedWidth, this->AlignedHeight }, DpbSlotCount, this->DpbImage);
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateBuffer(
  VkBufferUsageFlags usage, VkDeviceSize size, bool hostVisible, Buffer& buffer)
{
  auto& vk = this->Device.Functions;
  VkDevice device = this->Device.Device;
  VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
  if (usage & VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR)
  {
    info.pNext = &this->ProfileList;
  }
  info.size = size;
  info.usage = usage;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VTK_VK_CHECK(vk.vkCreateBuffer(device, &info, nullptr, &buffer.Handle));
  buffer.Size = size;

  VkBufferMemoryRequirementsInfo2 reqInfo = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2 };
  reqInfo.buffer = buffer.Handle;
  VkMemoryRequirements2 reqs = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
  vk.vkGetBufferMemoryRequirements2(device, &reqInfo, &reqs);
  const VkMemoryPropertyFlags required =
    hostVisible ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) : 0;
  if (!this->AllocateMemory(
        reqs.memoryRequirements, required, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer.Memory))
  {
    return false;
  }
  VkBindBufferMemoryInfo bind = { VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO };
  bind.buffer = buffer.Handle;
  bind.memory = buffer.Memory;
  VTK_VK_CHECK(vk.vkBindBufferMemory2(device, 1, &bind));
  if (hostVisible)
  {
    VTK_VK_CHECK(vk.vkMapMemory(device, buffer.Memory, 0, VK_WHOLE_SIZE, 0, &buffer.Mapped));
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkVulkanEncoderInternals::DestroyBuffer(Buffer& buffer)
{
  auto& vk = this->Device.Functions;
  VkDevice device = this->Device.Device;
  if (buffer.Mapped != nullptr)
  {
    vk.vkUnmapMemory(device, buffer.Memory);
  }
  if (buffer.Handle != VK_NULL_HANDLE)
  {
    vk.vkDestroyBuffer(device, buffer.Handle, nullptr);
  }
  if (buffer.Memory != VK_NULL_HANDLE)
  {
    vk.vkFreeMemory(device, buffer.Memory, nullptr);
  }
  buffer = {};
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateBuffers()
{
  const VkDeviceSize lumaBytes =
    static_cast<VkDeviceSize>(this->AlignedWidth) * this->AlignedHeight;
  const VkDeviceSize bitstreamSize =
    AlignUp<VkDeviceSize>(std::max<VkDeviceSize>(lumaBytes * 3, 2u << 20),
      this->Capabilities.minBitstreamBufferSizeAlignment);
  return this->CreateBuffer(
    VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR, bitstreamSize, true, this->BitstreamBuffer);
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateQueryPool()
{
  VkQueryPoolVideoEncodeFeedbackCreateInfoKHR feedback = {
    VK_STRUCTURE_TYPE_QUERY_POOL_VIDEO_ENCODE_FEEDBACK_CREATE_INFO_KHR, &this->Profile
  };
  feedback.encodeFeedbackFlags = VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BUFFER_OFFSET_BIT_KHR |
    VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BYTES_WRITTEN_BIT_KHR;
  VkQueryPoolCreateInfo info = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, &feedback };
  info.queryType = VK_QUERY_TYPE_VIDEO_ENCODE_FEEDBACK_KHR;
  info.queryCount = 1;
  VTK_VK_CHECK(this->Device.Functions.vkCreateQueryPool(
    this->Device.Device, &info, nullptr, &this->QueryPool));
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::CreateCommandResources()
{
  auto& vk = this->Device.Functions;
  VkDevice device = this->Device.Device;
  auto createPool = [&](uint32_t family, VkCommandPool& pool, VkCommandBuffer& cmd) -> bool
  {
    VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = family;
    VTK_VK_CHECK(vk.vkCreateCommandPool(device, &poolInfo, nullptr, &pool));
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VTK_VK_CHECK(vk.vkAllocateCommandBuffers(device, &allocInfo, &cmd));
    return true;
  };
  if (!createPool(
        this->Device.EncodeQueueFamily, this->EncodeCommandPool, this->EncodeCommandBuffer) ||
    !createPool(
      this->Device.ComputeQueueFamily, this->ComputeCommandPool, this->ComputeCommandBuffer))
  {
    return false;
  }
  VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
  VTK_VK_CHECK(vk.vkCreateFence(device, &fenceInfo, nullptr, &this->Fence));
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::Submit(VkQueue queue, VkCommandBuffer cmd, VkSemaphore wait,
  std::initializer_list<VkSemaphore> signals, bool blockUntilDone)
{
  auto& vk = this->Device.Functions;
  VkCommandBufferSubmitInfo cmdInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
  cmdInfo.commandBuffer = cmd;
  VkSemaphoreSubmitInfo waitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
  waitInfo.semaphore = wait;
  waitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  std::vector<VkSemaphoreSubmitInfo> signalInfos;
  for (VkSemaphore signal : signals)
  {
    VkSemaphoreSubmitInfo signalInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
    signalInfo.semaphore = signal;
    signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    signalInfos.push_back(signalInfo);
  }

  VkSubmitInfo2 submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
  submit.commandBufferInfoCount = 1;
  submit.pCommandBufferInfos = &cmdInfo;
  submit.waitSemaphoreInfoCount = wait != VK_NULL_HANDLE ? 1 : 0;
  submit.pWaitSemaphoreInfos = &waitInfo;
  submit.signalSemaphoreInfoCount = static_cast<uint32_t>(signalInfos.size());
  submit.pSignalSemaphoreInfos = signalInfos.data();
  VTK_VK_CHECK(
    vk.vkQueueSubmit2(queue, 1, &submit, blockUntilDone ? this->Fence : VK_NULL_HANDLE));
  if (blockUntilDone)
  {
    VTK_VK_CHECK(vk.vkWaitForFences(this->Device.Device, 1, &this->Fence, VK_TRUE, UINT64_MAX));
    VTK_VK_CHECK(vk.vkResetFences(this->Device.Device, 1, &this->Fence));
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::TransitionDpbToEncodeLayout()
{
  auto& vk = this->Device.Functions;
  VkCommandBufferBeginInfo begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VTK_VK_CHECK(vk.vkBeginCommandBuffer(this->EncodeCommandBuffer, &begin));
  CmdImageBarrier(vk, this->EncodeCommandBuffer,
    MakeImageBarrier(this->DpbImage.Handle, this->DpbImage.Layers, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR, VK_PIPELINE_STAGE_2_NONE, 0,
      VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR,
      VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR | VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR));
  VTK_VK_CHECK(vk.vkEndCommandBuffer(this->EncodeCommandBuffer));
  if (!this->Submit(this->Device.EncodeQueue, this->EncodeCommandBuffer, VK_NULL_HANDLE, {},
        /*blockUntilDone=*/true))
  {
    return false;
  }
  VTK_VK_CHECK(vk.vkResetCommandPool(this->Device.Device, this->EncodeCommandPool, 0));
  return true;
}

//------------------------------------------------------------------------------
// The input image starts VK_IMAGE_LAYOUT_UNDEFINED, but GL's first WaitFree() assumes it is
// already in GL_LAYOUT_GENERAL_EXT (the layout every Vulkan->GL handoff leaves it in), so this
// one-time transition runs -- and signals FreeSemaphore -- before the frame loop.
bool vtkVulkanEncoderInternals::InitializeInteropImageLayout()
{
  auto& vk = this->Device.Functions;
  VkCommandBufferBeginInfo begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VTK_VK_CHECK(vk.vkBeginCommandBuffer(this->ComputeCommandBuffer, &begin));
  CmdImageBarrier(vk, this->ComputeCommandBuffer,
    MakeImageBarrier(this->InputImage.Handle, 1, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_2_NONE, 0, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
      0));
  VTK_VK_CHECK(vk.vkEndCommandBuffer(this->ComputeCommandBuffer));
  if (!this->Submit(this->Device.ComputeQueue, this->ComputeCommandBuffer, VK_NULL_HANDLE,
        { this->FreeSemaphore }, /*blockUntilDone=*/true))
  {
    return false;
  }
  VTK_VK_CHECK(vk.vkResetCommandPool(this->Device.Device, this->ComputeCommandPool, 0));
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::EncodeFrame(bool forceKeyFrame, EncodedFrame& out)
{
  auto& vk = this->Device.Functions;
  if (!this->IsReady())
  {
    return false;
  }
  const auto t0 = std::chrono::high_resolution_clock::now();
  const bool keyFrame = forceKeyFrame || this->FrameIndexInGop == 0 || !this->HaveReference;
  if (keyFrame)
  {
    this->FrameIndexInGop = 0;
    this->FrameNum = 0;
  }

  // The previous frame's encode was waited on with the fence, and it in turn waited on the
  // previous conversion, so both command buffers are idle and safe to reset.
  VTK_VK_CHECK(vk.vkResetCommandPool(this->Device.Device, this->ComputeCommandPool, 0));
  VTK_VK_CHECK(vk.vkResetCommandPool(this->Device.Device, this->EncodeCommandPool, 0));
  VkCommandBufferBeginInfo begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VTK_VK_CHECK(vk.vkBeginCommandBuffer(this->ComputeCommandBuffer, &begin));
  this->RecordConvert(this->ComputeCommandBuffer);
  VTK_VK_CHECK(vk.vkEndCommandBuffer(this->ComputeCommandBuffer));
  if (!this->Submit(this->Device.ComputeQueue, this->ComputeCommandBuffer, this->ReadySemaphore,
        { this->FreeSemaphore, this->ConvertDoneSemaphore }, /*blockUntilDone=*/false))
  {
    return false;
  }

  VTK_VK_CHECK(vk.vkBeginCommandBuffer(this->EncodeCommandBuffer, &begin));
  // The semaphore wait already made the conversion's writes visible; only the layout changes.
  CmdImageBarrier(vk, this->EncodeCommandBuffer,
    MakeImageBarrier(this->SourceImage.Handle, 1,
      this->DirectConvert ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
      VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR,
      VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR));
  this->RecordEncode(this->EncodeCommandBuffer, keyFrame);
  VTK_VK_CHECK(vk.vkEndCommandBuffer(this->EncodeCommandBuffer));
  if (!this->Submit(this->Device.EncodeQueue, this->EncodeCommandBuffer,
        this->ConvertDoneSemaphore, {}, /*blockUntilDone=*/true))
  {
    return false;
  }
  if (!this->FinishEncodedFrame(keyFrame, out))
  {
    return false;
  }
  this->dtEncode = std::chrono::high_resolution_clock::now() - t0;
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::FinishEncodedFrame(bool keyFrame, EncodedFrame& out)
{
  if (!this->ReadBackBitstream(out))
  {
    return false;
  }
  if (this->NeedsSliceHeaderPaddingFix)
  {
    this->FixSliceHeaderPadding(out.Bitstream);
  }
  out.IsKeyFrame = keyFrame;
  out.PresentationTS = this->FrameCount;
  if (keyFrame)
  {
    out.Bitstream.insert(
      out.Bitstream.begin(), this->ParameterSetBytes.begin(), this->ParameterSetBytes.end());
    ++this->IdrPicId;
  }

  this->HaveReference = true;
  this->CurrentSlot = 1 - this->CurrentSlot;
  this->FrameNum = (this->FrameNum + 1) & ((1u << Log2MaxFrameNum) - 1);
  this->FrameIndexInGop = (this->FrameIndexInGop + 1) % std::max(1, this->Settings.GopSize);
  ++this->FrameCount;
  return true;
}

//------------------------------------------------------------------------------
// Recorded on the compute queue. OpenGL leaves the input image in GL_LAYOUT_GENERAL_EXT after
// signaling ReadySemaphore, and the semaphore wait makes its writes visible, so the image needs
// no barrier here and stays in GENERAL for the next GL blit. The shader converts it into the
// NV12 source image's planes -- directly, or via the R8/RG8 plane images plus a copy. Every
// target is fully overwritten, so previous contents/layouts are discarded.
void vtkVulkanEncoderInternals::RecordConvert(VkCommandBuffer cmd)
{
  auto& vk = this->Device.Functions;
  const VkImage targets[2] = { this->DirectConvert ? this->SourceImage.Handle
                                                   : this->PlaneImages[0].Handle,
    this->DirectConvert ? VK_NULL_HANDLE : this->PlaneImages[1].Handle };
  for (const VkImage target : targets)
  {
    if (target == VK_NULL_HANDLE)
    {
      continue;
    }
    CmdImageBarrier(vk, cmd,
      MakeImageBarrier(target, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT));
  }

  vk.vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->ConvertPipeline);
  vk.vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->ConvertPipelineLayout, 0,
    1, &this->ConvertDescriptorSet, 0, nullptr);
  const ConvertPushConstants constants = { static_cast<uint32_t>(this->Settings.Width),
    static_cast<uint32_t>(this->Settings.Height), this->Settings.FlipInput ? 1u : 0u };
  vk.vkCmdPushConstants(cmd, this->ConvertPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
    sizeof(constants), &constants);
  // One invocation per 2x2 block, i.e. per chroma texel.
  const uint32_t blocksX = this->AlignedWidth / 2;
  const uint32_t blocksY = this->AlignedHeight / 2;
  vk.vkCmdDispatch(cmd, AlignUp(blocksX, ConvertWorkgroupSize) / ConvertWorkgroupSize,
    AlignUp(blocksY, ConvertWorkgroupSize) / ConvertWorkgroupSize, 1);
  if (this->DirectConvert)
  {
    return;
  }

  for (const Image& plane : this->PlaneImages)
  {
    CmdImageBarrier(vk, cmd,
      MakeImageBarrier(plane.Handle, 1, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_READ_BIT));
  }
  CmdImageBarrier(vk, cmd,
    MakeImageBarrier(this->SourceImage.Handle, 1, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
      VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
      VK_ACCESS_2_TRANSFER_WRITE_BIT));
  const VkImageAspectFlags planeAspects[2] = { VK_IMAGE_ASPECT_PLANE_0_BIT,
    VK_IMAGE_ASPECT_PLANE_1_BIT };
  for (uint32_t plane = 0; plane < 2; ++plane)
  {
    VkImageCopy region = {};
    region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.dstSubresource = { planeAspects[plane], 0, 0, 1 };
    region.extent = { this->AlignedWidth >> plane, this->AlignedHeight >> plane, 1 };
    vk.vkCmdCopyImage(cmd, this->PlaneImages[plane].Handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      this->SourceImage.Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  }
}

//------------------------------------------------------------------------------
void vtkVulkanEncoderInternals::RecordEncode(VkCommandBuffer cmd, bool keyFrame)
{
  auto& vk = this->Device.Functions;
  const int32_t setupSlot = this->CurrentSlot;
  const int32_t refSlot = 1 - setupSlot;
  const VkExtent2D codedExtent = { static_cast<uint32_t>(this->Settings.Width),
    static_cast<uint32_t>(this->Settings.Height) };

  VkVideoPictureResourceInfoKHR dpbResources[DpbSlotCount];
  VkVideoReferenceSlotInfoKHR boundSlots[DpbSlotCount];
  for (uint32_t i = 0; i < DpbSlotCount; ++i)
  {
    dpbResources[i] = { VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR };
    dpbResources[i].codedExtent = codedExtent;
    dpbResources[i].baseArrayLayer = i;
    dpbResources[i].imageViewBinding = this->DpbImage.View;
    // Only active slots may carry an index; inactive ones just bind their resource.
    boundSlots[i] = { VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR };
    boundSlots[i].slotIndex = this->SlotActive[i] ? static_cast<int32_t>(i) : -1;
    boundSlots[i].pPictureResource = &dpbResources[i];
  }

  vk.vkCmdResetQueryPool(cmd, this->QueryPool, 0, 1);

  VkVideoBeginCodingInfoKHR beginInfo = { VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR };
  beginInfo.pNext = this->RateControlApplied ? &this->RateControl : nullptr;
  beginInfo.videoSession = this->Session;
  beginInfo.videoSessionParameters = this->SessionParameters;
  beginInfo.referenceSlotCount = DpbSlotCount;
  beginInfo.pReferenceSlots = boundSlots;
  vk.vkCmdBeginVideoCodingKHR(cmd, &beginInfo);

  if (!this->RateControlApplied)
  {
    const auto& s = this->Settings;
    const auto& caps = this->EncodeCapabilities;
    VkVideoEncodeRateControlModeFlagBitsKHR mode = VK_VIDEO_ENCODE_RATE_CONTROL_MODE_DEFAULT_KHR;
    switch (s.RateControl)
    {
      case RateControlMode::ConstantBitrate:
        mode = VK_VIDEO_ENCODE_RATE_CONTROL_MODE_CBR_BIT_KHR;
        break;
      case RateControlMode::VariableBitrate:
        mode = VK_VIDEO_ENCODE_RATE_CONTROL_MODE_VBR_BIT_KHR;
        break;
      case RateControlMode::ConstantQp:
        mode = VK_VIDEO_ENCODE_RATE_CONTROL_MODE_DISABLED_BIT_KHR;
        break;
    }
    if ((caps.rateControlModes & mode) == 0)
    {
      vtkLogF(WARNING, "Rate control mode 0x%x unsupported; using driver default.", mode);
      mode = VK_VIDEO_ENCODE_RATE_CONTROL_MODE_DEFAULT_KHR;
    }
    const bool bitrateMode = mode == VK_VIDEO_ENCODE_RATE_CONTROL_MODE_CBR_BIT_KHR ||
      mode == VK_VIDEO_ENCODE_RATE_CONTROL_MODE_VBR_BIT_KHR;

    const auto& h264Caps = this->H264Capabilities;
    const int32_t minQp = std::clamp(s.MinQp, h264Caps.minQp, h264Caps.maxQp);
    const int32_t maxQp = std::clamp(s.MaxQp, h264Caps.minQp, h264Caps.maxQp);
    this->H264RateControlLayer = {
      VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_LAYER_INFO_KHR
    };
    // An equal min/max pair would pin the QP and defeat bitrate control; leave that to the driver.
    if (minQp < maxQp)
    {
      this->H264RateControlLayer.useMinQp = VK_TRUE;
      this->H264RateControlLayer.minQp = { minQp, minQp, minQp };
      this->H264RateControlLayer.useMaxQp = VK_TRUE;
      this->H264RateControlLayer.maxQp = { maxQp, maxQp, maxQp };
    }
    this->RateControlLayer = { VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR,
      &this->H264RateControlLayer };
    const uint64_t maxBitrate =
      std::min<uint64_t>(caps.maxBitrate, std::max(s.MaxBitrate, s.AverageBitrate));
    this->RateControlLayer.averageBitrate = mode == VK_VIDEO_ENCODE_RATE_CONTROL_MODE_CBR_BIT_KHR
      ? maxBitrate
      : std::min(s.AverageBitrate, maxBitrate);
    this->RateControlLayer.maxBitrate = maxBitrate;
    this->RateControlLayer.frameRateNumerator = s.FrameRateNumerator;
    this->RateControlLayer.frameRateDenominator = s.FrameRateDenominator;

    this->H264RateControl = { VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_INFO_KHR };
    this->H264RateControl.flags = VK_VIDEO_ENCODE_H264_RATE_CONTROL_REGULAR_GOP_BIT_KHR |
      VK_VIDEO_ENCODE_H264_RATE_CONTROL_REFERENCE_PATTERN_FLAT_BIT_KHR;
    this->H264RateControl.gopFrameCount = static_cast<uint32_t>(std::max(1, s.GopSize));
    this->H264RateControl.idrPeriod = this->H264RateControl.gopFrameCount;
    this->H264RateControl.consecutiveBFrameCount = 0;
    this->H264RateControl.temporalLayerCount = 1;

    this->RateControl = { VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR };
    this->RateControl.rateControlMode = mode;
    if (bitrateMode)
    {
      this->RateControl.pNext = &this->H264RateControl;
      this->RateControl.layerCount = 1;
      this->RateControl.pLayers = &this->RateControlLayer;
      this->RateControl.virtualBufferSizeInMs = s.LowDelay ? 200 : 1000;
      this->RateControl.initialVirtualBufferSizeInMs = this->RateControl.virtualBufferSizeInMs / 2;
    }

    VkVideoCodingControlInfoKHR control = { VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR,
      &this->RateControl };
    control.flags =
      VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR | VK_VIDEO_CODING_CONTROL_ENCODE_RATE_CONTROL_BIT_KHR;
    vk.vkCmdControlVideoCodingKHR(cmd, &control);
    this->RateControlApplied = true;
  }

  const int32_t poc =
    static_cast<int32_t>((2 * this->FrameIndexInGop) & ((1u << Log2MaxPocLsb) - 1));
  const StdVideoH264PictureType picType =
    keyFrame ? STD_VIDEO_H264_PICTURE_TYPE_IDR : STD_VIDEO_H264_PICTURE_TYPE_P;

  StdVideoEncodeH264ReferenceListsInfo refLists = {};
  std::memset(
    refLists.RefPicList0, STD_VIDEO_H264_NO_REFERENCE_PICTURE, sizeof(refLists.RefPicList0));
  std::memset(
    refLists.RefPicList1, STD_VIDEO_H264_NO_REFERENCE_PICTURE, sizeof(refLists.RefPicList1));
  if (!keyFrame)
  {
    refLists.RefPicList0[0] = static_cast<uint8_t>(refSlot);
  }

  StdVideoEncodeH264PictureInfo pictureInfo = {};
  pictureInfo.flags.IdrPicFlag = keyFrame;
  pictureInfo.flags.is_reference = 1;
  pictureInfo.idr_pic_id = keyFrame ? this->IdrPicId : 0;
  pictureInfo.primary_pic_type = picType;
  pictureInfo.frame_num = this->FrameNum;
  pictureInfo.PicOrderCnt = poc;
  pictureInfo.pRefLists = &refLists;

  StdVideoEncodeH264SliceHeader sliceHeader = {};
  sliceHeader.slice_type = keyFrame ? STD_VIDEO_H264_SLICE_TYPE_I : STD_VIDEO_H264_SLICE_TYPE_P;
  sliceHeader.cabac_init_idc = STD_VIDEO_H264_CABAC_INIT_IDC_0;
  sliceHeader.disable_deblocking_filter_idc = STD_VIDEO_H264_DISABLE_DEBLOCKING_FILTER_IDC_DISABLED;

  VkVideoEncodeH264NaluSliceInfoKHR sliceInfo = {
    VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_NALU_SLICE_INFO_KHR
  };
  sliceInfo.constantQp =
    this->RateControl.rateControlMode == VK_VIDEO_ENCODE_RATE_CONTROL_MODE_DISABLED_BIT_KHR
    ? std::clamp(this->Settings.Qp, this->H264Capabilities.minQp, this->H264Capabilities.maxQp)
    : 0;
  sliceInfo.pStdSliceHeader = &sliceHeader;

  VkVideoEncodeH264PictureInfoKHR h264Picture = {
    VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PICTURE_INFO_KHR
  };
  h264Picture.naluSliceEntryCount = 1;
  h264Picture.pNaluSliceEntries = &sliceInfo;
  h264Picture.pStdPictureInfo = &pictureInfo;

  StdVideoEncodeH264ReferenceInfo setupReference = {};
  setupReference.primary_pic_type = picType;
  setupReference.FrameNum = this->FrameNum;
  setupReference.PicOrderCnt = poc;
  VkVideoEncodeH264DpbSlotInfoKHR setupDpbInfo = {
    VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_DPB_SLOT_INFO_KHR
  };
  setupDpbInfo.pStdReferenceInfo = &setupReference;
  VkVideoReferenceSlotInfoKHR setupSlotInfo = { VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR,
    &setupDpbInfo };
  setupSlotInfo.slotIndex = setupSlot;
  setupSlotInfo.pPictureResource = &dpbResources[setupSlot];

  VkVideoEncodeH264DpbSlotInfoKHR refDpbInfo = {
    VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_DPB_SLOT_INFO_KHR
  };
  refDpbInfo.pStdReferenceInfo = &this->ReferenceInfo;
  VkVideoReferenceSlotInfoKHR refSlotInfo = { VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR,
    &refDpbInfo };
  refSlotInfo.slotIndex = refSlot;
  refSlotInfo.pPictureResource = &dpbResources[refSlot];

  VkVideoEncodeInfoKHR encodeInfo = { VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR, &h264Picture };
  encodeInfo.dstBuffer = this->BitstreamBuffer.Handle;
  encodeInfo.dstBufferOffset = 0;
  encodeInfo.dstBufferRange = this->BitstreamBuffer.Size;
  encodeInfo.srcPictureResource = { VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR };
  encodeInfo.srcPictureResource.codedExtent = codedExtent;
  encodeInfo.srcPictureResource.imageViewBinding = this->SourceImage.View;
  encodeInfo.pSetupReferenceSlot = &setupSlotInfo;
  encodeInfo.referenceSlotCount = keyFrame ? 0 : 1;
  encodeInfo.pReferenceSlots = keyFrame ? nullptr : &refSlotInfo;

  vk.vkCmdBeginQuery(cmd, this->QueryPool, 0, 0);
  vk.vkCmdEncodeVideoKHR(cmd, &encodeInfo);
  vk.vkCmdEndQuery(cmd, this->QueryPool, 0);

  // Reconstructed picture written this frame is read as reference by the next one.
  CmdImageBarrier(vk, cmd,
    MakeImageBarrier(this->DpbImage.Handle, this->DpbImage.Layers,
      VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR, VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR,
      VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR, VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR,
      VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR, VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR));

  VkVideoEndCodingInfoKHR endInfo = { VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR };
  vk.vkCmdEndVideoCodingKHR(cmd, &endInfo);

  CmdBufferBarrier(vk, cmd, this->BitstreamBuffer.Handle, VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR,
    VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR, VK_PIPELINE_STAGE_2_HOST_BIT,
    VK_ACCESS_2_HOST_READ_BIT);

  this->ReferenceInfo = setupReference;
  this->SlotActive[setupSlot] = true;
}

//------------------------------------------------------------------------------
// Mesa < 26.0 ANV H.264 encoder workaround.
//
// vk_video_encode_h264_slice_header() in Mesa's Vulkan runtime terminates the slice header
// with rbsp_trailing_bits() (a stop bit plus zero padding up to the next byte boundary) and
// reports its length in whole bytes; ANV feeds exactly those bytes to the PAK, which then
// appends the macroblock data after the padding rather than right after the header's last
// bit. Decoders read the stop bit as the start of macroblock 0 and the picture decodes as
// garbage. Fixed in Mesa 26.0 (the header length is passed on in bits).
//
// The output can be repaired after the fact: the slice header syntax depends only on SPS/PPS
// fields this class chose, so parse it to find its length in bits, cut the 1..8 padding bits
// out and shift the macroblock data back into place. Emulation prevention bytes are removed
// before the shift and re-inserted afterwards because the shift changes byte values.
namespace
{
class RbspReader
{
public:
  explicit RbspReader(const std::vector<unsigned char>& rbsp)
    : Data(rbsp)
  {
  }

  uint32_t U(unsigned count)
  {
    uint32_t value = 0;
    for (unsigned i = 0; i < count; ++i)
    {
      if (this->Pos >= this->Data.size() * 8)
      {
        this->Ok = false;
        return 0;
      }
      value = (value << 1) | ((this->Data[this->Pos >> 3] >> (7 - (this->Pos & 7))) & 1u);
      ++this->Pos;
    }
    return value;
  }

  uint32_t UE()
  {
    unsigned leadingZeros = 0;
    while (this->Ok && this->U(1) == 0)
    {
      if (++leadingZeros > 31)
      {
        this->Ok = false;
        return 0;
      }
    }
    return leadingZeros == 0 ? 0 : ((1u << leadingZeros) - 1 + this->U(leadingZeros));
  }

  int32_t SE()
  {
    const uint32_t k = this->UE();
    return (k & 1u) ? static_cast<int32_t>((k + 1) / 2) : -static_cast<int32_t>(k / 2);
  }

  bool Ok = true;
  std::size_t Pos = 0;

private:
  const std::vector<unsigned char>& Data;
};

// Length in bits of nal_unit_header + slice_header() at the start of `rbsp` (an unescaped
// type 1 or 5 NAL unit), or 0 when the syntax is not one this encoder asks for
// (SP/SI slices, weighted prediction tables).
std::size_t SliceHeaderBitLength(const std::vector<unsigned char>& rbsp,
  const StdVideoH264SequenceParameterSet& sps, const StdVideoH264PictureParameterSet& pps)
{
  RbspReader r(rbsp);
  const uint32_t nalHeader = r.U(8);
  const uint32_t nalRefIdc = (nalHeader >> 5) & 3u;
  const bool idr = (nalHeader & 0x1Fu) == 5;

  r.UE(); // first_mb_in_slice
  const uint32_t sliceType = r.UE() % 5;
  r.UE(); // pic_parameter_set_id
  if (sps.flags.separate_colour_plane_flag)
  {
    r.U(2); // colour_plane_id
  }
  r.U(sps.log2_max_frame_num_minus4 + 4); // frame_num
  bool fieldPic = false;
  if (!sps.flags.frame_mbs_only_flag)
  {
    fieldPic = r.U(1) != 0;
    if (fieldPic)
    {
      r.U(1); // bottom_field_flag
    }
  }
  if (idr)
  {
    r.UE(); // idr_pic_id
  }
  if (sps.pic_order_cnt_type == 0)
  {
    r.U(sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
    if (pps.flags.bottom_field_pic_order_in_frame_present_flag && !fieldPic)
    {
      r.SE(); // delta_pic_order_cnt_bottom
    }
  }
  else if (sps.pic_order_cnt_type == 1 && !sps.flags.delta_pic_order_always_zero_flag)
  {
    r.SE();
    if (pps.flags.bottom_field_pic_order_in_frame_present_flag && !fieldPic)
    {
      r.SE();
    }
  }
  if (pps.flags.redundant_pic_cnt_present_flag)
  {
    r.UE();
  }

  const bool isP = sliceType == STD_VIDEO_H264_SLICE_TYPE_P;
  const bool isB = sliceType == STD_VIDEO_H264_SLICE_TYPE_B;
  const bool isI = sliceType == STD_VIDEO_H264_SLICE_TYPE_I;
  if (!isP && !isB && !isI)
  {
    return 0;
  }
  if (isB)
  {
    r.U(1); // direct_spatial_mv_pred_flag
  }
  if (isP || isB)
  {
    if (r.U(1)) // num_ref_idx_active_override_flag
    {
      r.UE();
      if (isB)
      {
        r.UE();
      }
    }
    // ref_pic_list_modification()
    for (int list = 0; list < (isB ? 2 : 1); ++list)
    {
      if (r.U(1))
      {
        for (unsigned ops = 0; r.Ok; ++ops)
        {
          const uint32_t idc = r.UE(); // modification_of_pic_nums_idc
          if (idc == 3)
          {
            break;
          }
          if (idc > 3 || ops > 32)
          {
            return 0;
          }
          r.UE();
        }
      }
    }
  }
  if ((pps.flags.weighted_pred_flag && isP) || (pps.weighted_bipred_idc == 1 && isB))
  {
    return 0; // pred_weight_table(): never requested
  }
  if (nalRefIdc != 0) // dec_ref_pic_marking()
  {
    if (idr)
    {
      r.U(2); // no_output_of_prior_pics_flag, long_term_reference_flag
    }
    else if (r.U(1)) // adaptive_ref_pic_marking_mode_flag
    {
      for (unsigned ops = 0; r.Ok; ++ops)
      {
        const uint32_t mmco = r.UE();
        if (mmco == 0)
        {
          break;
        }
        if (mmco > 6 || ops > 32)
        {
          return 0;
        }
        if (mmco == 1 || mmco == 3)
        {
          r.UE(); // difference_of_pic_nums_minus1
        }
        if (mmco == 2)
        {
          r.UE(); // long_term_pic_num
        }
        if (mmco == 3 || mmco == 6)
        {
          r.UE(); // long_term_frame_idx
        }
        if (mmco == 4)
        {
          r.UE(); // max_long_term_frame_idx_plus1
        }
      }
    }
  }
  if (pps.flags.entropy_coding_mode_flag && !isI)
  {
    r.UE(); // cabac_init_idc
  }
  r.SE(); // slice_qp_delta
  if (pps.flags.deblocking_filter_control_present_flag)
  {
    if (r.UE() != 1) // disable_deblocking_filter_idc
    {
      r.SE(); // slice_alpha_c0_offset_div2
      r.SE(); // slice_beta_offset_div2
    }
  }
  if (pps.flags.entropy_coding_mode_flag)
  {
    while (r.Ok && (r.Pos & 7) != 0)
    {
      if (r.U(1) != 1) // cabac_alignment_one_bit
      {
        return 0;
      }
    }
  }
  return r.Ok ? r.Pos : 0;
}

std::vector<unsigned char> RemoveEmulationPrevention(
  const unsigned char* begin, const unsigned char* end)
{
  std::vector<unsigned char> out;
  out.reserve(end - begin);
  unsigned zeros = 0;
  for (; begin != end; ++begin)
  {
    if (zeros >= 2 && *begin == 3)
    {
      zeros = 0;
      continue;
    }
    out.push_back(*begin);
    zeros = (*begin == 0) ? zeros + 1 : 0;
  }
  return out;
}

void AppendWithEmulationPrevention(
  const std::vector<unsigned char>& rbsp, std::vector<unsigned char>& out)
{
  unsigned zeros = 0;
  for (const unsigned char c : rbsp)
  {
    if (zeros >= 2 && c <= 3)
    {
      out.push_back(3);
      zeros = 0;
    }
    out.push_back(c);
    zeros = (c == 0) ? zeros + 1 : 0;
  }
}

// Deletes the bits [first, first + count) from `rbsp`; first + count must be byte aligned.
void EraseBits(std::vector<unsigned char>& rbsp, std::size_t first, unsigned count)
{
  const std::size_t dstByte = first / 8;
  const unsigned keep = first % 8; // leading bits of rbsp[dstByte] that belong to the header
  const std::size_t srcByte = (first + count) / 8;
  if (keep == 0)
  {
    rbsp.erase(rbsp.begin() + dstByte, rbsp.begin() + srcByte);
    return;
  }
  std::size_t dst = dstByte;
  auto carry = static_cast<unsigned char>(rbsp[dstByte] & (0xFFu << (8 - keep)));
  for (std::size_t src = srcByte; src < rbsp.size(); ++src, ++dst)
  {
    rbsp[dst] = static_cast<unsigned char>(carry | (rbsp[src] >> keep));
    carry = static_cast<unsigned char>(rbsp[src] << (8 - keep));
  }
  rbsp[dst++] = carry;
  rbsp.resize(dst);
}

// Rewrites the type 1/5 NAL unit [begin, end) (no start code) without the rbsp trailing
// bits Mesa put between slice_header() and slice_data(). Returns false if the header could
// not be parsed or the expected padding was not there; `out` is left untouched then.
bool StripSliceHeaderPadding(const unsigned char* begin, const unsigned char* end,
  const StdVideoH264SequenceParameterSet& sps, const StdVideoH264PictureParameterSet& pps,
  std::vector<unsigned char>& out)
{
  std::vector<unsigned char> rbsp = RemoveEmulationPrevention(begin, end);
  const std::size_t headerBits = SliceHeaderBitLength(rbsp, sps, pps);
  if (headerBits == 0)
  {
    return false;
  }
  // The padding runs from the end of the header to the next byte boundary; a header that
  // already ends on a boundary got a whole 0x80 byte.
  const unsigned paddingBits = 8 - (headerBits % 8);
  if ((headerBits + paddingBits) / 8 >= rbsp.size())
  {
    return false;
  }
  RbspReader check(rbsp);
  check.Pos = headerBits;
  if (check.U(1) != 1 || check.U(paddingBits - 1) != 0)
  {
    return false; // no stop bit + zero padding here: not the bug this works around
  }
  EraseBits(rbsp, headerBits, paddingBits);
  // The shift can leave all-zero bytes after the slice's own trailing stop bit.
  const std::size_t headerBytes = (headerBits + 7) / 8;
  while (rbsp.size() > headerBytes && rbsp.back() == 0)
  {
    rbsp.pop_back();
  }
  AppendWithEmulationPrevention(rbsp, out);
  return true;
}
} // namespace

//------------------------------------------------------------------------------
void vtkVulkanEncoderInternals::FixSliceHeaderPadding(std::vector<unsigned char>& stream) const
{
  const std::size_t n = stream.size();
  auto isStartCode = [&stream, n](std::size_t i) {
    return i + 2 < n && stream[i] == 0 && stream[i + 1] == 0 && stream[i + 2] == 1;
  };
  std::vector<unsigned char> out;
  out.reserve(n);
  std::size_t pos = 0;
  while (pos < n)
  {
    // Copy up to and including the next start code verbatim.
    std::size_t sc = pos;
    while (sc < n && !isStartCode(sc))
    {
      ++sc;
    }
    if (sc >= n)
    {
      out.insert(out.end(), stream.begin() + pos, stream.end());
      break;
    }
    const std::size_t payload = sc + 3;
    out.insert(out.end(), stream.begin() + pos, stream.begin() + payload);
    std::size_t next = payload;
    while (next < n && !isStartCode(next))
    {
      ++next;
    }
    // Zero bytes before the next start code are trailing_zero_8bits / the 4-byte start
    // code's leading zero, not NAL payload.
    std::size_t payloadEnd = next;
    while (payloadEnd > payload && stream[payloadEnd - 1] == 0)
    {
      --payloadEnd;
    }
    const unsigned nalType = payload < n ? (stream[payload] & 0x1Fu) : 0u;
    const bool fixed = (nalType == 1 || nalType == 5) &&
      ::StripSliceHeaderPadding(
        stream.data() + payload, stream.data() + payloadEnd, this->Sps, this->Pps, out);
    if (!fixed)
    {
      if (nalType == 1 || nalType == 5)
      {
        vtkLog(WARNING,
          "Could not locate the slice header padding Mesa < 26.0 inserts; slice left as is.");
      }
      out.insert(out.end(), stream.begin() + payload, stream.begin() + payloadEnd);
    }
    out.insert(out.end(), stream.begin() + payloadEnd, stream.begin() + next);
    pos = next;
  }
  stream.swap(out);
}

//------------------------------------------------------------------------------
bool vtkVulkanEncoderInternals::ReadBackBitstream(EncodedFrame& out)
{
  auto& vk = this->Device.Functions;
  struct
  {
    uint32_t Offset;
    uint32_t BytesWritten;
    int32_t Status;
  } result = {};
  VTK_VK_CHECK(vk.vkGetQueryPoolResults(this->Device.Device, this->QueryPool, 0, 1, sizeof(result),
    &result, sizeof(result), VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_WITH_STATUS_BIT_KHR));
  if (result.Status != VK_QUERY_RESULT_STATUS_COMPLETE_KHR)
  {
    vtkLogF(ERROR, "Video encode failed with query status %d", result.Status);
    return false;
  }
  if (static_cast<VkDeviceSize>(result.Offset) + result.BytesWritten > this->BitstreamBuffer.Size)
  {
    vtkLog(ERROR, "Encoder reported more bytes than the bitstream buffer holds.");
    return false;
  }
  VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
  range.memory = this->BitstreamBuffer.Memory;
  range.size = VK_WHOLE_SIZE;
  VTK_VK_CHECK(vk.vkInvalidateMappedMemoryRanges(this->Device.Device, 1, &range));
  const auto* data =
    static_cast<const unsigned char*>(this->BitstreamBuffer.Mapped) + result.Offset;
  out.Bitstream.assign(data, data + result.BytesWritten);
  return true;
}
