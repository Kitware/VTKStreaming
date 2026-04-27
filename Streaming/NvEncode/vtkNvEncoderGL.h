/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNvEncoderGL.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkNvEncoderGL_h
#define vtkNvEncoderGL_h

#include "vtkVideoEncoder.h"

#include "vtkStreamingNvEncodeModule.h" // for export macro

#include <memory> // for ivar

class vtkNvEncoderInternals;
class vtkCUDADriverLoader;
class vtkOpenGLRenderWindow;
class vtkGenericOpenGLResourceFreeCallback;

class VTKSTREAMINGNVENCODE_EXPORT vtkNvEncoderGL : public vtkVideoEncoder
{
public:
  vtkTypeMacro(vtkNvEncoderGL, vtkVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override{};
  static vtkNvEncoderGL* New();

  vtkSetClampMacro(Preset, int, 1, 7);
  vtkGetMacro(Preset, int);

  vtkSetClampMacro(Profile, int, 1, 11);
  vtkGetMacro(Profile, int);

  vtkSetClampMacro(Tune, int, 1, 4);
  vtkGetMacro(Tune, int);

  bool IsHardwareAccelerated() const noexcept override { return true; }
  vtkIdType GetLastEncodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override;

  static bool CheckAvailability() noexcept;

protected:
  vtkNvEncoderGL();
  ~vtkNvEncoderGL() override;

  int Preset = 1;  // NV_ENC_PRESET_P1_GUID
  int Profile = 1; // NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID
  int Tune = 2;    // NV_ENC_TUNING_INFO_LOW_LATENCY

  vtkGenericOpenGLResourceFreeCallback* ResourceCallback = nullptr;

  bool InitializeInternal() override;
  void ShutdownInternal() override;

  bool SetupEncoderFrame(int, int) override;
  void TearDownEncoderFrame() override;

  VTKVideoEncoderResultType EncodeInternal(vtkSmartPointer<vtkRawVideoFrame> frame) override;
  VTKVideoEncoderResultType SendEOS() override;

  bool AllocateInputBuffers();
  void ReleaseInputBuffers();
  void ReleaseGLResources(vtkWindow* window);

private:
  vtkNvEncoderGL(const vtkNvEncoderGL&) = delete;
  void operator=(const vtkNvEncoderGL&) = delete;

  class vtkCUDAContext;
  std::unique_ptr<vtkNvEncoderInternals> Internals;
  std::unique_ptr<vtkCUDADriverLoader> CUDADriverLoader;
  std::unique_ptr<vtkCUDAContext> CUDAInstance;
  bool CUDADriverAvailable = false;
};

#endif // vtkNvEncoderGL_h
