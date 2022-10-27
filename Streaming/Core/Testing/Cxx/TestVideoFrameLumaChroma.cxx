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

#include "vtkLogger.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRawVideoFrame.h"
#include "vtkStreamingTestUtility.h"

int TestVideoFrameLumaChroma(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  (void)argc;
  (void)argv;
  {
    const char* name = " 1. NV12, 3x4 estSize = 96";
    vtkLogF(TRACE, "%s", name);
    success = true;

    const auto estSize = vtkRawVideoFrame::GetEstimatedSize(3, 4, VTKPixelFormatType::VTKPF_NV12);
    success &= (estSize == 96);

    if (!success)
    {
      vtkLogF(ERROR, "Failed | %s | estSize : %d != 96", name, estSize);
      return 1;
    }
    else
    {
      vtkLogF(INFO, "Success | %s | estSize : %d == 96", name, estSize);
    }
  }

  {
    const char* name = " 2. NV12, 8x9 estSize = 192";
    vtkLogF(TRACE, "%s", name);
    success = true;

    const auto estSize = vtkRawVideoFrame::GetEstimatedSize(8, 9, VTKPixelFormatType::VTKPF_NV12);
    success &= (estSize == 192);

    if (!success)
    {
      vtkLogF(ERROR, "Failed | %s | estSize : %d != 192", name, estSize);
      return 1;
    }
    else
    {
      vtkLogF(INFO, "Success | %s | estSize : %d == 192", name, estSize);
    }
  }

  {
    const char* name = " 3. NV12, 1920x1076 estSize = 3110400";
    vtkLogF(TRACE, "%s", name);
    success = true;

    const auto estSize =
      vtkRawVideoFrame::GetEstimatedSize(1920, 1076, VTKPixelFormatType::VTKPF_NV12);
    success &= (estSize == 3110400);

    if (!success)
    {
      vtkLogF(ERROR, "Failed | %s | estSize : %d != 3110400", name, estSize);
      return 1;
    }
    else
    {
      vtkLogF(INFO, "Success | %s | estSize : %d == 3110400", name, estSize);
    }
  }

  {
    const char* name = " 3. RGBA, 4x5 estSize = 80";
    vtkLogF(TRACE, "%s", name);
    success = true;

    const auto estSize = vtkRawVideoFrame::GetEstimatedSize(4, 5, VTKPixelFormatType::VTKPF_RGBA32);
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

    const auto estSize = vtkRawVideoFrame::GetEstimatedSize(4, 5, VTKPixelFormatType::VTKPF_RGB24);
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
