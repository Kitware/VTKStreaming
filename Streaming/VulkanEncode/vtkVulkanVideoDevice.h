// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
/**
 * @class   vtkVulkanVideoDevice
 * @brief   owns a VkInstance/VkDevice with video encode queues.
 *
 * The first encode-capable GPU is selected.
 */

#ifndef vtkVulkanVideoDevice_h
#define vtkVulkanVideoDevice_h

#include "vtkStreamingVulkanEncodeModule.h"

#include "vtkVulkanFunctions.h"

#include <cstdint>

class VTKSTREAMINGVULKANENCODE_NO_EXPORT vtkVulkanVideoDevice
{
public:
  vtkVulkanVideoDevice();
  ~vtkVulkanVideoDevice();
  vtkVulkanVideoDevice(const vtkVulkanVideoDevice&) = delete;
  void operator=(const vtkVulkanVideoDevice&) = delete;

  /**
   * True when the Vulkan loader is present and at least one physical device can
   * encode H.264. Creates and destroys a throwaway instance.
   */
  static bool CheckAvailability() noexcept;

  ///@{
  /**
   * `deviceUUID` (VkPhysicalDeviceIDProperties::deviceUUID, 16 bytes) restricts the choice
   * to that GPU -- the one driving the OpenGL context, so memory can be shared with it.
   * nullptr accepts any encode-capable GPU.
   */
  bool CreateInstance();
  bool SelectPhysicalDevice(const unsigned char* deviceUUID = nullptr);
  bool CreateDevice();
  void Destroy();
  ///@}

  [[nodiscard]] bool IsReady() const { return this->Device != VK_NULL_HANDLE; }
  /**
   * Picks a memory type matching `required | preferred` if one exists, else just `required`.
   */
  bool FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags required,
    VkMemoryPropertyFlags preferred, uint32_t& index) const;

  vtkVulkanFunctions Functions;
  VkInstance Instance = VK_NULL_HANDLE;
  VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
  VkDevice Device = VK_NULL_HANDLE;
  VkPhysicalDeviceMemoryProperties MemoryProperties = {};
  VkPhysicalDeviceProperties Properties = {};
  // driverID/driverInfo/conformanceVersion of the selected device; pNext is not meaningful.
  VkPhysicalDeviceDriverProperties DriverProperties = {};

  // The compute family runs the RGBA->NV12 conversion (and any copy); it may equal the encode
  // family when the driver's encode queue also exposes compute.
  uint32_t EncodeQueueFamily = VK_QUEUE_FAMILY_IGNORED;
  uint32_t ComputeQueueFamily = VK_QUEUE_FAMILY_IGNORED;
  VkQueue EncodeQueue = VK_NULL_HANDLE;
  VkQueue ComputeQueue = VK_NULL_HANDLE;

private:
  static bool HasRequiredExtensions(vtkVulkanFunctions& vk, VkPhysicalDevice device);
  static bool HasRequiredFeatures(vtkVulkanFunctions& vk, VkPhysicalDevice device);
  static bool FindQueueFamilies(vtkVulkanFunctions& vk, VkPhysicalDevice device,
    uint32_t& encodeFamily, uint32_t& computeFamily);
};

#endif // vtkVulkanVideoDevice_h
// VTK-HeaderTest-Exclude: vtkVulkanVideoDevice.h
