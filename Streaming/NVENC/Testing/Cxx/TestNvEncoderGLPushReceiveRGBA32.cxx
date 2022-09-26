/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncoderGLPushReceiveRGBA32.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises NvEnc h.264 encoder with RGBA32 inputs.

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCylinderSource.h"
#include "vtkLogger.h"
#include "vtkNamedColors.h"
#include "vtkNvEncoderGL.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkVideoCodecTypes.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <string>

#define WRITE_CHUNKS 0

int TestNvEncoderGLPushReceiveRGBA32(int argc, char* argv[])
{
  bool success = true;
  int width = 320, height = 240;

  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "spinnin_cylinder_320x240_100_frames.h264");
  std::string baselineFile = filename;
  delete[] filename;

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkCylinderSource> cyl;
  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkNamedColors> colors;

  // Set the background color.
  std::array<unsigned char, 4> bkg{ { 26, 51, 102, 255 } };
  colors->SetColor("BkgColor", bkg.data());

  mapper->SetInputConnection(cyl->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(colors->GetColor4d("Tomato").GetData());
  actor->RotateX(30.0);
  actor->RotateY(-45.0);

  ren->AddActor(actor);
  ren->SetBackground(colors->GetColor3d("BkgColor").GetData());

  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->AddRenderer(ren);
  renWin->SetSize(width, height);
  iren->SetRenderWindow(renWin);
  iren->Initialize();
  iren->Render();

  vtkNew<vtkNvEncoderGL> enc;
  enc->SetGraphicsContext(renWin);
  enc->SetCodec(VTKVideoCodecType::VTKVC_H264);
  enc->SetWidth(width);
  enc->SetHeight(height);
  enc->AsyncModeOff();
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);

  vtkNew<vtkCallbackCommand> exitCallback;
  exitCallback->SetClientData(enc);
  exitCallback->SetCallback(
    [](vtkObject* iren_ptr, unsigned long, void* enc_ptr, void*)
    {
      // drain needs an opengl context so it can release the resources.
      auto encoder = reinterpret_cast<vtkVideoEncoder*>(enc_ptr);
      auto result = encoder->Drain();
      (void)result;
      auto iren = reinterpret_cast<vtkRenderWindowInteractor*>(iren_ptr);
      encoder->Shutdown();
      iren->TerminateApp();
    });
  iren->AddObserver(vtkCommand::ExitEvent, exitCallback);

  int frameId = 0;
  std::vector<uint8_t> bitstream;
  while (true)
  {
    double azimuth = (frameId % 36) * 10;
    ren->GetActiveCamera()->Azimuth(vtkMath::RadiansFromDegrees(azimuth));
    renWin->Render();
    if (frameId > 100)
    {
      iren->ExitEvent();
    }
    if (iren->GetDone())
    {
      break;
    }
    vtkOpenGLCheckErrors("error uploading data to gl texture");

    auto result = enc->EncodeDisplay();

    vtkLog(TRACE, << vtkVideoProcessingStatusTypeUtilities::ToString(result.first));
    success &= !(result.second.empty() || result.second[0] == nullptr);
    if (!success)
    {
      break;
    }
    auto data = result.second[0]->GetData()->GetPointer(0);
    auto size = result.second[0]->GetSize();
    vtkLogF(INFO, "Recv %d bytes", size);
    for (std::size_t i = 0; i < size; ++i)
    {
      bitstream.push_back(data[i]);
    }
    ++frameId;
  }

  std::ifstream baseline;
  vtkLogF(INFO, "Read %s", baselineFile.c_str());
  baseline.open(baselineFile, std::ios::in | std::ios::binary);
  baseline.ignore(std::numeric_limits<std::streamsize>::max());

  const std::size_t size1 = baseline.gcount();
  const std::size_t size2 = bitstream.size();
  vtkLogF(TRACE, "%zu, %zu", size1, size2);
  success = size1 == size2;

  baseline.clear();
  baseline.seekg(0, std::ios_base::beg);

  std::vector<uint8_t> baseline_ptr(size1, 0);
  baseline.read(reinterpret_cast<char*>(baseline_ptr.data()), size1);

  for (std::size_t i1 = 0, i2 = 0; i1 < size1 && i2 < size2 && success; ++i1 && ++i2)
  {
    vtkLogF(TRACE, "%d, %d", int(baseline_ptr[i1]), int(bitstream[i2]));
    success &= (baseline_ptr[i1] == bitstream[i2]);
  }
  if (!success)
  {
    filename = vtkTestUtilities::ExpandFileNameWithArgOrEnvOrDefault("-T", argc, argv,
      "VTKSTREAMING_DATA_ROOT", "Temporary", "spinnin_cylinder_320x240_100_frames.h264");
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    file.write(reinterpret_cast<char*>(bitstream.data()), bitstream.size());
  }
  return success ? 0 : 1;
}
