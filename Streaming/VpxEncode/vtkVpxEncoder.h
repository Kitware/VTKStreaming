/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVpxEncoder.h

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkVpxEncoder_h
#define vtkVpxEncoder_h

#include "vtkVideoEncoder.h"

#include "vtkStreamingVpxEncodeModule.h" // for export macro

#include <memory> // for ivar

class VTKSTREAMINGVPXENCODE_EXPORT vtkVpxEncoder : public vtkVideoEncoder
{
public:
  vtkTypeMacro(vtkVpxEncoder, vtkVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkVpxEncoder* New();

  vtkSetMacro(ErrorResilient, bool);
  vtkGetMacro(ErrorResilient, bool);

  vtkSetClampMacro(TargetCPUUsage, int, -9, 9);
  vtkGetMacro(TargetCPUUsage, int);

  vtkSetClampMacro(Sharpness, int, 0, 7);
  vtkGetMacro(Sharpness, int);

  vtkSetClampMacro(MaxIntraBitrateFactor, float, 1, 10);
  vtkGetMacro(MaxIntraBitrateFactor, float);

  vtkSetClampMacro(MaxInterBitrateFactor, float, 1, 10);
  vtkGetMacro(MaxInterBitrateFactor, float);

  vtkSetClampMacro(TileColumns, int, 0, 6);
  vtkGetMacro(TileColumns, int);

  vtkSetClampMacro(TileRows, int, 0, 6);
  vtkGetMacro(TileRows, int);

  vtkSetMacro(RowBasedMultiThreading, bool);
  vtkGetMacro(RowBasedMultiThreading, bool);

  bool IsHardwareAccelerated() const noexcept override { return false; }
  vtkIdType GetLastEncodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override;

protected:
  vtkVpxEncoder();
  ~vtkVpxEncoder() override;

  bool ErrorResilient = false;
  int TargetCPUUsage = 5;
  int Sharpness = 4;
  float MaxIntraBitrateFactor = 4.5;
  float MaxInterBitrateFactor = 4.5;
  int TileColumns = 6;
  int TileRows = 0;
  bool RowBasedMultiThreading = true;

  bool InitializeInternal() override;
  void ShutdownInternal() override;

  bool SetupEncoderFrame(int width, int height) override;
  void TearDownEncoderFrame() override;

  VTKVideoEncoderResultType EncodeInternal(vtkSmartPointer<vtkRawVideoFrame> frame) override;
  VTKVideoEncoderResultType SendEOS() override;

private:
  vtkVpxEncoder(const vtkVpxEncoder&) = delete;
  void operator=(const vtkVpxEncoder&) = delete;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};
#endif
