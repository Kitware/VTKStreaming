/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixelFormats.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPixelFormats_h
#define vtkPixelFormats_h

enum VTKPixelFormat
{
  // Packed format
  RGBA32, // r,g,b,a 8:8:8:8, 32 bpp Ex: 2x2 -> rgba|rgba|rgba|rgba
  RGB24,  // r,g,b   8:8:8, 24 bpp Ex: 2x2 -> rgb|rgb|rgb|rgb

  // Planar format
  // Y: Luminance, U: Chromatic blue difference (Cb) and V: Chromatic red difference (Cr)
  YUV420P, // y,Cr,Cr   4:2:0, 12 bpp Ex: 2x2 -> yyyy|uu|vv
  NV12,    // y,Cb,Cr   4:2:0, 12 bpp Ex: 2x2 -> yyyy|uvuv
};

#endif // vtkPixelFormats_h
