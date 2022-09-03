/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCodecTypes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkCodecTypes_h
#define vtkCodecTypes_h

// For new codecs, please insert above MaxNumberOfSupportedCodecs

enum VTKCodecType
{
  VP9,
  AV1,
  H264,
  H265,
  MaxNumberOfSupportedCodecs
};

#endif // vtkCodecTypes_h
