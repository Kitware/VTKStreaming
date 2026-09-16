// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "vtkVulkanVideoDevice.h"

#include "vtkLogger.h"

#include <cstring>
#include <vector>

namespace
{
const char* const RequiredDeviceExtensions[] = {
  VK_KHR_VIDEO_QUEUE_EXTENSION_NAME,
  VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME,
  VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME,
  VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
  VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
};
constexpr uint32_t RequiredApiVersion = VK_API_VERSION_1_3;
}

//------------------------------------------------------------------------------
vtkVulkanVideoDevice::vtkVulkanVideoDevice() = default;

//------------------------------------------------------------------------------
vtkVulkanVideoDevice::~vtkVulkanVideoDevice()
{
  this->Destroy();
}

//------------------------------------------------------------------------------
bool vtkVulkanVideoDevice::CheckAvailability() noexcept
{
  vtkVulkanVideoDevice device;
  return device.CreateInstance() && device.SelectPhysicalDevice();
}

//------------------------------------------------------------------------------
bool vtkVulkanVideoDevice::CreateInstance()
{
  auto& vk = this->Functions;
  if (!vk.LoadVulkanLibrary())
  {
    return false;
  }
  uint32_t loaderVersion = VK_API_VERSION_1_0;
  vk.vkEnumerateInstanceVersion(&loaderVersion);
  if (loaderVersion < RequiredApiVersion)
  {
    vtkLogF(ERROR, "Vulkan loader %u.%u is older than 1.3", VK_VERSION_MAJOR(loaderVersion),
      VK_VERSION_MINOR(loaderVersion));
    return false;
  }
  VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
  appInfo.pApplicationName = "vtkstreaming";
  appInfo.pEngineName = "vtkstreaming";
  appInfo.apiVersion = RequiredApiVersion;
  VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
  createInfo.pApplicationInfo = &appInfo;
  const VkResult result = vk.vkCreateInstance(&createInfo, nullptr, &this->Instance);
  if (result != VK_SUCCESS)
  {
    vtkLogF(ERROR, "vkCreateInstance failed (%d)", static_cast<int>(result));
    return false;
  }
  return vk.LoadInstanceFunctions(this->Instance);
}

//------------------------------------------------------------------------------
bool vtkVulkanVideoDevice::HasRequiredExtensions(vtkVulkanFunctions& vk, VkPhysicalDevice device)
{
  uint32_t count = 0;
  vk.vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> available(count);
  vk.vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());
  for (const char* required : RequiredDeviceExtensions)
  {
    bool found = false;
    for (const auto& ext : available)
    {
      if (!std::strcmp(ext.extensionName, required))
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      vtkLogF(TRACE, "Missing device extension %s", required);
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanVideoDevice::FindQueueFamilies(
  vtkVulkanFunctions& vk, VkPhysicalDevice device, uint32_t& encodeFamily, uint32_t& computeFamily)
{
  uint32_t count = 0;
  vk.vkGetPhysicalDeviceQueueFamilyProperties2(device, &count, nullptr);
  std::vector<VkQueueFamilyProperties2> families(
    count, { VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 });
  std::vector<VkQueueFamilyVideoPropertiesKHR> videoProps(
    count, { VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR });
  for (uint32_t i = 0; i < count; ++i)
  {
    families[i].pNext = &videoProps[i];
  }
  vk.vkGetPhysicalDeviceQueueFamilyProperties2(device, &count, families.data());

  encodeFamily = computeFamily = VK_QUEUE_FAMILY_IGNORED;
  for (uint32_t i = 0; i < count; ++i)
  {
    const VkQueueFlags flags = families[i].queueFamilyProperties.queueFlags;
    if ((flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) &&
      (videoProps[i].videoCodecOperations & VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR) &&
      encodeFamily == VK_QUEUE_FAMILY_IGNORED)
    {
      encodeFamily = i;
    }
  }
  if (encodeFamily == VK_QUEUE_FAMILY_IGNORED)
  {
    return false;
  }
  // The RGBA->NV12 conversion is a compute dispatch (plus, on drivers that cannot write the
  // encode source image from a shader, a copy; compute queues support transfers implicitly).
  // Video encode families rarely have compute, so this usually lands on the graphics queue.
  if (families[encodeFamily].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
  {
    computeFamily = encodeFamily;
    return true;
  }
  for (uint32_t i = 0; i < count; ++i)
  {
    if (families[i].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
      computeFamily = i;
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkVulkanVideoDevice::HasRequiredFeatures(vtkVulkanFunctions& vk, VkPhysicalDevice device)
{
  VkPhysicalDeviceSynchronization2Features sync2 = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES
  };
  VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &sync2 };
  vk.vkGetPhysicalDeviceFeatures2(device, &features);
  // The conversion shader stores to r8/rg8 storage images.
  return sync2.synchronization2 && features.features.shaderStorageImageExtendedFormats;
}

//------------------------------------------------------------------------------
bool vtkVulkanVideoDevice::SelectPhysicalDevice(const unsigned char* deviceUUID)
{
  auto& vk = this->Functions;
  if (this->Instance == VK_NULL_HANDLE)
  {
    return false;
  }
  uint32_t count = 0;
  vk.vkEnumeratePhysicalDevices(this->Instance, &count, nullptr);
  std::vector<VkPhysicalDevice> candidates(count);
  vk.vkEnumeratePhysicalDevices(this->Instance, &count, candidates.data());

  for (VkPhysicalDevice candidate : candidates)
  {
    VkPhysicalDeviceDriverProperties driverProps = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES
    };
    VkPhysicalDeviceIDProperties idProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
      &driverProps };
    VkPhysicalDeviceProperties2 props = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      &idProps };
    vk.vkGetPhysicalDeviceProperties2(candidate, &props);
    const auto& p = props.properties;
    if (deviceUUID != nullptr && std::memcmp(idProps.deviceUUID, deviceUUID, VK_UUID_SIZE) != 0)
    {
      vtkLogF(TRACE, "%s: not the GPU driving the OpenGL context", p.deviceName);
      continue;
    }
    if (p.apiVersion < RequiredApiVersion)
    {
      vtkLogF(TRACE, "%s: Vulkan %u.%u < 1.3", p.deviceName, VK_VERSION_MAJOR(p.apiVersion),
        VK_VERSION_MINOR(p.apiVersion));
      continue;
    }
    uint32_t encodeFamily, computeFamily;
    if (!this->HasRequiredExtensions(vk, candidate) || !this->HasRequiredFeatures(vk, candidate) ||
      !this->FindQueueFamilies(vk, candidate, encodeFamily, computeFamily))
    {
      vtkLogF(TRACE, "%s: no H.264 encode support", p.deviceName);
      continue;
    }
    vtkLogF(TRACE, "Selected %s (encode qf=%u, compute qf=%u)", p.deviceName, encodeFamily,
      computeFamily);
    this->PhysicalDevice = candidate;
    this->EncodeQueueFamily = encodeFamily;
    this->ComputeQueueFamily = computeFamily;
    this->Properties = p;
    this->DriverProperties = driverProps;
    this->DriverProperties.pNext = nullptr;
    vk.vkGetPhysicalDeviceMemoryProperties(candidate, &this->MemoryProperties);
    return true;
  }
  vtkLogF(TRACE, "No Vulkan device with H.264 video encode support");
  return false;
}

//------------------------------------------------------------------------------
bool vtkVulkanVideoDevice::CreateDevice()
{
  auto& vk = this->Functions;
  if (this->PhysicalDevice == VK_NULL_HANDLE)
  {
    return false;
  }
  const float priority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queueInfos;
  VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
  queueInfo.queueCount = 1;
  queueInfo.pQueuePriorities = &priority;
  queueInfo.queueFamilyIndex = this->EncodeQueueFamily;
  queueInfos.push_back(queueInfo);
  if (this->ComputeQueueFamily != this->EncodeQueueFamily)
  {
    queueInfo.queueFamilyIndex = this->ComputeQueueFamily;
    queueInfos.push_back(queueInfo);
  }

  VkPhysicalDeviceSynchronization2Features sync2 = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES
  };
  sync2.synchronization2 = VK_TRUE;
  VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
  features.pNext = &sync2;
  features.features.shaderStorageImageExtendedFormats = VK_TRUE;

  VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
  createInfo.pNext = &features;
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
  createInfo.pQueueCreateInfos = queueInfos.data();
  createInfo.enabledExtensionCount =
    sizeof(RequiredDeviceExtensions) / sizeof(RequiredDeviceExtensions[0]);
  createInfo.ppEnabledExtensionNames = RequiredDeviceExtensions;
  const VkResult result =
    vk.vkCreateDevice(this->PhysicalDevice, &createInfo, nullptr, &this->Device);
  if (result != VK_SUCCESS)
  {
    vtkLogF(ERROR, "vkCreateDevice failed (%d)", static_cast<int>(result));
    return false;
  }
  if (!vk.LoadDeviceFunctions(this->Device))
  {
    return false;
  }
  vk.vkGetDeviceQueue(this->Device, this->EncodeQueueFamily, 0, &this->EncodeQueue);
  vk.vkGetDeviceQueue(this->Device, this->ComputeQueueFamily, 0, &this->ComputeQueue);
  return true;
}

//------------------------------------------------------------------------------
void vtkVulkanVideoDevice::Destroy()
{
  auto& vk = this->Functions;
  if (this->Device != VK_NULL_HANDLE)
  {
    vk.vkDeviceWaitIdle(this->Device);
    vk.vkDestroyDevice(this->Device, nullptr);
    this->Device = VK_NULL_HANDLE;
    this->EncodeQueue = this->ComputeQueue = VK_NULL_HANDLE;
  }
  if (this->Instance != VK_NULL_HANDLE)
  {
    vk.vkDestroyInstance(this->Instance, nullptr);
    this->Instance = VK_NULL_HANDLE;
  }
  this->PhysicalDevice = VK_NULL_HANDLE;
}

//------------------------------------------------------------------------------
bool vtkVulkanVideoDevice::FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags required,
  VkMemoryPropertyFlags preferred, uint32_t& index) const
{
  for (VkMemoryPropertyFlags wanted : { required | preferred, required })
  {
    for (uint32_t i = 0; i < this->MemoryProperties.memoryTypeCount; ++i)
    {
      if ((typeBits & (1u << i)) &&
        (this->MemoryProperties.memoryTypes[i].propertyFlags & wanted) == wanted)
      {
        index = i;
        return true;
      }
    }
  }
  return false;
}
