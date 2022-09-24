/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVideoFrameLumaChroma.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises the calculation and luma chroma offset and size of vtkRawVideoFrame

#include "vtkCPUVideoFrame.h"
#include "vtkLogger.h"
#include "vtkPixelFormatTypes.h"

int TestVideoFrameLumaChroma(int argc, char* argv[])
{
  bool success = true;
  (void)argc;
  (void)argv;
  {
    const char* name = " 1. NV12, 4x4 estSize = 24";
    vtkLogF(TRACE, "%s", name);
    success = true;

    const auto estSize = vtkCPUVideoFrame::GetEstimatedSize(4, 4, VTKPixelFormatType::VTKPF_NV12);
    success &= (estSize == 24);

    if (!success)
    {
      vtkLogF(ERROR, "Failed | %s | estSize : %d != 24", name, estSize);
      return 1;
    }
    else
    {
      vtkLogF(INFO, "Success | %s | estSize : %d == 24", name, estSize);
    }
  }

  {
    const char* name = " 2. NV12, 4x5 estSize = 32";
    vtkLogF(TRACE, "%s", name);
    success = true;

    const auto estSize = vtkCPUVideoFrame::GetEstimatedSize(4, 5, VTKPixelFormatType::VTKPF_NV12);
    success &= (estSize == 32);

    if (!success)
    {
      vtkLogF(ERROR, "Failed | %s | estSize : %d != 32", name, estSize);
      return 1;
    }
    else
    {
      vtkLogF(INFO, "Success | %s | estSize : %d == 32", name, estSize);
    }
  }

  {
    const char* name = " 3. RGBA, 4x5 estSize = 80";
    vtkLogF(TRACE, "%s", name);
    success = true;

    vtkNew<vtkCPUVideoFrame> frame;

    const auto estSize = vtkCPUVideoFrame::GetEstimatedSize(4, 5, VTKPixelFormatType::VTKPF_RGBA32);
    success &= (estSize == 80);

    if (!success)
    {
      vtkLogF(ERROR, "Failed | %s | estSize : %d != 80", name, estSize);
      return 1;
    }
    else
    {
      vtkLogF(INFO, "Success | %s | estSize : %d == 80", name, estSize);
    }
  }

  {
    const char* name = " 4. RGB, 4x5 estSize = 60";
    vtkLogF(TRACE, "%s", name);
    success = true;

    vtkNew<vtkCPUVideoFrame> frame;

    const auto estSize = vtkCPUVideoFrame::GetEstimatedSize(4, 5, VTKPixelFormatType::VTKPF_RGB24);
    success &= (estSize == 60);

    if (!success)
    {
      vtkLogF(ERROR, "Failed | %s | estSize : %d != 60", name, estSize);
      return 1;
    }
    else
    {
      vtkLogF(INFO, "Success | %s | estSize : %d == 60", name, estSize);
    }
  }
  return 0;
}
