// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef vtkVulkanDynamicLoader_h
#define vtkVulkanDynamicLoader_h

#include <stdlib.h>

#if defined(_WIN32) &&                                                                             \
  (!defined(VTKSTREAMING_VULKAN_LOAD_FUNC) || !defined(VTKSTREAMING_VULKAN_SYM_FUNC) ||            \
    !defined(VTKSTREAMING_VULKAN_LIB_HANDLE))
#include <vtkWindows.h>
#endif

#ifndef VTKSTREAMING_VULKAN_LIB_HANDLE
#if defined(_WIN32)
#define VTKSTREAMING_VULKAN_LIB_HANDLE HMODULE
#else
#define VTKSTREAMING_VULKAN_LIB_HANDLE void*
#endif
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#define VTKSTREAMING_VULKAN_LIBNAME "vulkan-1.dll"
#else
#define VTKSTREAMING_VULKAN_LIBNAME "libvulkan.so.1"
#endif

#if !defined(VTKSTREAMING_VULKAN_LOAD_FUNC) || !defined(VTKSTREAMING_VULKAN_SYM_FUNC)
#ifdef _WIN32
#define VTKSTREAMING_VULKAN_LOAD_LIB(path) LoadLibrary(TEXT(path))
#define VTKSTREAMING_VULKAN_SYM_FUNC(lib, sym) GetProcAddress((lib), (sym))
#define VTKSTREAMING_VULKAN_FREE_LIB(lib) FreeLibrary(lib)
#else
#include <dlfcn.h>
#define VTKSTREAMING_VULKAN_LOAD_LIB(path) dlopen((path), RTLD_LAZY)
#define VTKSTREAMING_VULKAN_SYM_FUNC(lib, sym) dlsym((lib), (sym))
#define VTKSTREAMING_VULKAN_FREE_LIB(lib) dlclose(lib)
#endif
#endif

#endif
// VTK-HeaderTest-Exclude: vtkVulkanDynamicLoader.h
