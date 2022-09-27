/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNvDynamicLoader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkNvDynamicLoader_h
#define vtkNvDynamicLoader_h

#include <stdlib.h>

#if defined(_WIN32) &&                                                                             \
  (!defined(VTKSTREAMING_NV_LOAD_FUNC) || !defined(VTKSTREAMING_NV_SYM_FUNC) ||                    \
    !defined(VTKSTREAMING_NV_LIB_HANDLE))
#include <vtkWindows.h>
#endif

#ifndef VTKSTREAMING_NV_LIB_HANDLE
#if defined(_WIN32)
#define VTKSTREAMING_NV_LIB_HANDLE HMODULE
#else
#define VTKSTREAMING_NV_LIB_HANDLE void*
#endif
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#define VTKSTREAMING_CUDA_LIBNAME "nvcuda.dll"
#if defined(_WIN64) || defined(__CYGWIN64__)
#define VTKSTREAMING_NVENC_LIBNAME "nvEncodeAPI64.dll"
#else
#define VTKSTREAMING_NVENC_LIBNAME "nvEncodeAPI.dll"
#endif
#else
#define VTKSTREAMING_CUDA_LIBNAME "libcuda.so.1"
#define VTKSTREAMING_NVENC_LIBNAME "libnvidia-encode.so.1"
#endif

#if !defined(VTKSTREAMING_NV_LOAD_FUNC) || !defined(VTKSTREAMING_NV_SYM_FUNC)
#ifdef _WIN32
#define VTKSTREAMING_NV_LOAD_LIB(path) LoadLibrary(TEXT(path))
#define VTKSTREAMING_NV_SYM_FUNC(lib, sym) GetProcAddress((lib), (sym))
#define VTKSTREAMING_NV_FREE_LIB(lib) FreeLibrary(lib)
#else
#include <dlfcn.h>
#define VTKSTREAMING_NV_LOAD_LIB(path) dlopen((path), RTLD_LAZY)
#define VTKSTREAMING_NV_SYM_FUNC(lib, sym) dlsym((lib), (sym))
#define VTKSTREAMING_NV_FREE_LIB(lib) dlclose(lib)
#endif
#endif

#endif
// VTK-HeaderTest-Exclude: vtkNvDynamicLoader.h
