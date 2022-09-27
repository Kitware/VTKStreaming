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

struct vtkStreamingTestUtility
{
  static void SetLoggerVerbosityFromCli(int argc, char* argv[]);
};
#endif
// VTK-HeaderTest-Exclude: vtkStreamingTestUtility.h