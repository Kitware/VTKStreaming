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

class VTKSTREAMINGNVENCODE_EXPORT vtkNvEncoderGL : public vtkVideoEncoder
{
public:
  vtkTypeMacro(vtkNvEncoderGL, vtkVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override{};
  static vtkNvEncoderGL* New();

  bool IsHardwareAccelerated() const noexcept override { return true; }
  bool SupportsAsyncMode() const noexcept override { return false; }
  vtkIdType GetLastEncodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override;

protected:
  vtkNvEncoderGL();
  ~vtkNvEncoderGL() override;

  bool InitializeInternal() override;
  void ShutdownInternal() override;
  void FlushInternal() override;

  bool SetupEncoderFrame(int, int) override;
  bool NeedsNewEncoderFrame(int, int) override;
  void TearDownEncoderFrame() override;

  VTKVideoProcessingStatusType PushInternal(vtkRawVideoFrame* frame) override;
  VTKVideoEncoderResultType GetResultInternal() override;
  VTKVideoEncoderResultType EncodeInternal(vtkRawVideoFrame* frame) override;
  VTKVideoEncoderResultType DrainInternal() override;

  VTKVideoEncoderResultType EncodeDisplayInternal() override;

  bool AllocateInputBuffers();
  void ReleaseInputBuffers();
  void ReleaseGLResources();

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
