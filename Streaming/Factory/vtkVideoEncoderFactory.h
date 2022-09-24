/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoEncoderFactory.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkVideoEncoderFactory
 * @brief Creates a video encoder based on encoding capabilities on the machine and platform.
 *        For now, it is limited to FFmpeg encoders.
 */

#ifndef vtkVideoEncoderFactory_h
#define vtkVideoEncoderFactory_h

#include "vtkObject.h"

#include "vtkStreamingFactoryModule.h" // for export macro
#include "vtkVideoCodecTypes.h"        // for enum

#include <memory> // for ivar

class vtkVideoEncoder;

class VTKSTREAMINGFACTORY_EXPORT vtkVideoEncoderFactory : public vtkObject
{
public:
  static vtkVideoEncoderFactory* New();
  vtkTypeMacro(vtkVideoEncoderFactory, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum EncoderTypeEnum : int
  {
    None = -1,
    AMD,
    NVIDIA,
    Intel,
    Software,
    VideoToolbox,
    MaxTypes
  };

  void SetEncoderPreference(int encoderType);
  void SetEncoderPreference(EncoderTypeEnum encoderType);
  void ClearEncoderPreference();
  void PreferAMD();
  void PreferNVIDIA();
  void PreferIntel();
  void PreferSoftware();
  void PreferVideoToolbox();

  void LogAvailableEncoders();

  int GetFactoryBitrate() { return 10000000; }
  int GetFactoryGroupOfPicturesSize() { return 60; }
  int GetFactoryMaximumBFrames() { return 0; }
  int GetFactoryTimeBaseEnd() { return 240; }

  vtkVideoEncoder* NewEncoder();
  vtkVideoEncoder* NewEncoder(VTKVideoCodecType codec);
  vtkVideoEncoder* NewHardwareEncoder(VTKVideoCodecType codec);
  vtkVideoEncoder* NewSoftwareEncoder(VTKVideoCodecType codec);

protected:
  vtkVideoEncoderFactory();
  ~vtkVideoEncoderFactory() override;

  void Initialize();
  bool SupportsEncoderType(EncoderTypeEnum encoderType);
  bool SupportsEncoderTypeWithCodec(EncoderTypeEnum encoderType, VTKVideoCodecType codec);

  vtkVideoEncoder* CreateEncoder(EncoderTypeEnum encoderType);
  vtkVideoEncoder* CreateEncoderWithCodec(EncoderTypeEnum encoderType, VTKVideoCodecType codec);

private:
  vtkVideoEncoderFactory(const vtkVideoEncoderFactory&) = delete;
  void operator=(const vtkVideoEncoderFactory&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif // vtkVideoEncoderFactory_h
