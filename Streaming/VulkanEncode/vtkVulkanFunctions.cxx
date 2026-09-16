// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "vtkVulkanFunctions.h"

#include "vtkLogger.h"

//------------------------------------------------------------------------------
vtkVulkanFunctions::vtkVulkanFunctions() = default;

//------------------------------------------------------------------------------
vtkVulkanFunctions::~vtkVulkanFunctions()
{
  if (this->LibraryHandle != nullptr)
  {
    VTKSTREAMING_VULKAN_FREE_LIB(this->LibraryHandle);
  }
}

//------------------------------------------------------------------------------
bool vtkVulkanFunctions::LoadVulkanLibrary()
{
  if (this->LibraryHandle != nullptr)
  {
    return true;
  }
  this->LibraryHandle = VTKSTREAMING_VULKAN_LOAD_LIB(VTKSTREAMING_VULKAN_LIBNAME);
  if (this->LibraryHandle == nullptr)
  {
    vtkLogF(ERROR, "Failed to load %s", VTKSTREAMING_VULKAN_LIBNAME);
    return false;
  }
  this->vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
    VTKSTREAMING_VULKAN_SYM_FUNC(this->LibraryHandle, "vkGetInstanceProcAddr"));
  if (this->vkGetInstanceProcAddr == nullptr)
  {
    vtkLogF(ERROR, "%s has no vkGetInstanceProcAddr", VTKSTREAMING_VULKAN_LIBNAME);
    return false;
  }
#define VTK_VULKAN_LOAD(name)                                                                      \
  this->name = reinterpret_cast<PFN_##name>(this->vkGetInstanceProcAddr(nullptr, #name));          \
  if (this->name == nullptr)                                                                       \
  {                                                                                                \
    vtkLogF(ERROR, "Failed to resolve %s", #name);                                                 \
    return false;                                                                                  \
  }
  VTK_VULKAN_GLOBAL_FUNCTIONS(VTK_VULKAN_LOAD)
#undef VTK_VULKAN_LOAD
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanFunctions::LoadInstanceFunctions(VkInstance instance)
{
#define VTK_VULKAN_LOAD(name)                                                                      \
  this->name = reinterpret_cast<PFN_##name>(this->vkGetInstanceProcAddr(instance, #name));         \
  if (this->name == nullptr)                                                                       \
  {                                                                                                \
    vtkLogF(ERROR, "Failed to resolve %s", #name);                                                 \
    return false;                                                                                  \
  }
  VTK_VULKAN_INSTANCE_FUNCTIONS(VTK_VULKAN_LOAD)
#undef VTK_VULKAN_LOAD
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanFunctions::LoadDeviceFunctions(VkDevice device)
{
#define VTK_VULKAN_LOAD(name)                                                                      \
  this->name = reinterpret_cast<PFN_##name>(this->vkGetDeviceProcAddr(device, #name));             \
  if (this->name == nullptr)                                                                       \
  {                                                                                                \
    vtkLogF(ERROR, "Failed to resolve %s", #name);                                                 \
    return false;                                                                                  \
  }
  VTK_VULKAN_DEVICE_FUNCTIONS(VTK_VULKAN_LOAD)
#undef VTK_VULKAN_LOAD
  return true;
}
