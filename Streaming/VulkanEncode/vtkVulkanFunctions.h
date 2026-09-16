// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
/**
 * @class   vtkVulkanFunctions
 * @brief   Vulkan entry points resolved at runtime through vkGetInstanceProcAddr.
 *
 * Only `vkGetInstanceProcAddr` is looked up in the loader library; everything else
 * comes from `vkGetInstanceProcAddr`/`vkGetDeviceProcAddr`, so there is no link-time
 * dependency on Vulkan.
 */

#ifndef vtkVulkanFunctions_h
#define vtkVulkanFunctions_h

#include "vtkStreamingVulkanEncodeModule.h"

#include "vtkVulkanDynamicLoader.h"

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan_core.h"

// clang-format off
#define VTK_VULKAN_GLOBAL_FUNCTIONS(F)                                                            \
  F(vkEnumerateInstanceVersion)                                                                   \
  F(vkCreateInstance)

#define VTK_VULKAN_INSTANCE_FUNCTIONS(F)                                                          \
  F(vkDestroyInstance)                                                                            \
  F(vkEnumeratePhysicalDevices)                                                                   \
  F(vkGetPhysicalDeviceProperties2)                                                               \
  F(vkGetPhysicalDeviceFeatures2)                                                                 \
  F(vkGetPhysicalDeviceQueueFamilyProperties2)                                                    \
  F(vkGetPhysicalDeviceMemoryProperties)                                                          \
  F(vkGetPhysicalDeviceImageFormatProperties2)                                                    \
  F(vkEnumerateDeviceExtensionProperties)                                                         \
  F(vkCreateDevice)                                                                               \
  F(vkGetDeviceProcAddr)                                                                          \
  F(vkGetPhysicalDeviceVideoCapabilitiesKHR)                                                      \
  F(vkGetPhysicalDeviceVideoFormatPropertiesKHR)

#define VTK_VULKAN_DEVICE_FUNCTIONS(F)                                                            \
  F(vkDestroyDevice)                                                                              \
  F(vkGetDeviceQueue)                                                                             \
  F(vkDeviceWaitIdle)                                                                             \
  F(vkQueueSubmit2)                                                                               \
  F(vkQueueWaitIdle)                                                                              \
  F(vkCreateCommandPool)                                                                          \
  F(vkDestroyCommandPool)                                                                         \
  F(vkResetCommandPool)                                                                           \
  F(vkAllocateCommandBuffers)                                                                     \
  F(vkBeginCommandBuffer)                                                                         \
  F(vkEndCommandBuffer)                                                                           \
  F(vkCreateFence)                                                                                \
  F(vkDestroyFence)                                                                               \
  F(vkResetFences)                                                                                \
  F(vkWaitForFences)                                                                              \
  F(vkCreateSemaphore)                                                                            \
  F(vkDestroySemaphore)                                                                           \
  F(vkGetSemaphoreFdKHR)                                                                          \
  F(vkImportSemaphoreFdKHR)                                                                       \
  F(vkGetMemoryFdKHR)                                                                             \
  F(vkGetImageSubresourceLayout)                                                                  \
  F(vkAllocateMemory)                                                                             \
  F(vkFreeMemory)                                                                                 \
  F(vkMapMemory)                                                                                  \
  F(vkUnmapMemory)                                                                                \
  F(vkFlushMappedMemoryRanges)                                                                    \
  F(vkInvalidateMappedMemoryRanges)                                                               \
  F(vkCreateImage)                                                                                \
  F(vkDestroyImage)                                                                               \
  F(vkGetImageMemoryRequirements2)                                                                \
  F(vkBindImageMemory2)                                                                           \
  F(vkCreateImageView)                                                                            \
  F(vkDestroyImageView)                                                                           \
  F(vkCreateBuffer)                                                                               \
  F(vkDestroyBuffer)                                                                              \
  F(vkGetBufferMemoryRequirements2)                                                               \
  F(vkBindBufferMemory2)                                                                          \
  F(vkCreateQueryPool)                                                                            \
  F(vkDestroyQueryPool)                                                                           \
  F(vkGetQueryPoolResults)                                                                        \
  F(vkCmdResetQueryPool)                                                                          \
  F(vkCmdBeginQuery)                                                                              \
  F(vkCmdEndQuery)                                                                                \
  F(vkCmdPipelineBarrier2)                                                                        \
  F(vkCmdCopyImage)                                                                               \
  F(vkCreateShaderModule)                                                                         \
  F(vkDestroyShaderModule)                                                                        \
  F(vkCreateDescriptorSetLayout)                                                                  \
  F(vkDestroyDescriptorSetLayout)                                                                 \
  F(vkCreatePipelineLayout)                                                                       \
  F(vkDestroyPipelineLayout)                                                                      \
  F(vkCreateComputePipelines)                                                                     \
  F(vkDestroyPipeline)                                                                            \
  F(vkCreateDescriptorPool)                                                                       \
  F(vkDestroyDescriptorPool)                                                                      \
  F(vkAllocateDescriptorSets)                                                                     \
  F(vkUpdateDescriptorSets)                                                                       \
  F(vkCmdBindPipeline)                                                                            \
  F(vkCmdBindDescriptorSets)                                                                      \
  F(vkCmdPushConstants)                                                                           \
  F(vkCmdDispatch)                                                                                \
  F(vkCreateVideoSessionKHR)                                                                      \
  F(vkDestroyVideoSessionKHR)                                                                     \
  F(vkGetVideoSessionMemoryRequirementsKHR)                                                       \
  F(vkBindVideoSessionMemoryKHR)                                                                  \
  F(vkCreateVideoSessionParametersKHR)                                                            \
  F(vkDestroyVideoSessionParametersKHR)                                                           \
  F(vkGetEncodedVideoSessionParametersKHR)                                                        \
  F(vkCmdBeginVideoCodingKHR)                                                                     \
  F(vkCmdEndVideoCodingKHR)                                                                       \
  F(vkCmdControlVideoCodingKHR)                                                                   \
  F(vkCmdEncodeVideoKHR)
// clang-format on

class VTKSTREAMINGVULKANENCODE_NO_EXPORT vtkVulkanFunctions
{
public:
  vtkVulkanFunctions();
  ~vtkVulkanFunctions();
  vtkVulkanFunctions(const vtkVulkanFunctions&) = delete;
  void operator=(const vtkVulkanFunctions&) = delete;

  bool LoadVulkanLibrary();
  bool LoadInstanceFunctions(VkInstance instance);
  bool LoadDeviceFunctions(VkDevice device);
  bool IsLibraryLoaded() const { return this->LibraryHandle != nullptr; }

  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;

#define VTK_VULKAN_DECLARE(name) PFN_##name name = nullptr;
  VTK_VULKAN_GLOBAL_FUNCTIONS(VTK_VULKAN_DECLARE)
  VTK_VULKAN_INSTANCE_FUNCTIONS(VTK_VULKAN_DECLARE)
  VTK_VULKAN_DEVICE_FUNCTIONS(VTK_VULKAN_DECLARE)
#undef VTK_VULKAN_DECLARE

private:
  VTKSTREAMING_VULKAN_LIB_HANDLE LibraryHandle = nullptr;
};

#endif // vtkVulkanFunctions_h
// VTK-HeaderTest-Exclude: vtkVulkanFunctions.h
