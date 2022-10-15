/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamingTestUtility.h

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkStreamingTestUtility_h
#define vtkStreamingTestUtility_h

#include "vtkStreamingTestUtilitesModule.h"

class vtkUnsignedCharArray;

struct VTKSTREAMINGTESTUTILITES_EXPORT vtkStreamingTestUtility
{
  /** Problem: vtkLogger eats up `-vN` verbosity token
   * and it doesn't respect command line verbosity.
   * Solution: We use -V token to specify verbosity from the CLI.
   */
  static void SetLoggerVerbosityFromCli(int argc, char* argv[]);

  /**
   * Is the -I token present in command line arguments?
   */
  static bool GetInteractive(int argc, char** argv);

  /**
   *Generate strange color bars that can move across the screen.
   */
  static vtkUnsignedCharArray* GenerateRGBA32ColorBars(int width, int height, int shift = 0);
};
#endif
// VTK-HeaderTest-Exclude: vtkStreamingTestUtility.h