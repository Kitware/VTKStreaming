/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestJPEGDecoderPushReceiveRGBA32.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises JPEG video decoder with rgba32 outputs.
// It is disabled because it requres human intervention right now.
// The output jpeg files of TestJPEGDecoderPushReceiveRGBA32 will be inputs
// for this test.

#include "vtkCPUVideoFrame.h"
#include "vtkCompressedVideoPacket.h"
#include "vtkJPEGVideoDecoder.h"
#include "vtkLogger.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <fstream>
#include <iomanip>
#include <ios>
#include <limits>

#define WRITE_CHUNKS 1

int TestJPEGDecoderPushReceiveRGBA32(int argc, char* argv[])
{
  bool success = true;
  int width = 320, height = 240;

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;
  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->AddRenderer(ren);
  renWin->SetSize(width, height);
  ren->SetBackground(0, 0, 0);
  iren->SetRenderWindow(renWin);
  iren->Initialize();
  iren->Render();

  vtkNew<vtkJPEGVideoDecoder> dec;

#if !WRITE_CHUNKS
  std::ofstream outFile("cylinder.h264", std::ofstream::out | std::ofstream::binary);
#endif

  vtkNew<vtkOpenGLVideoFrame> dstFrame;
  dstFrame->SetContext(renWin);

  std::ifstream inFile;
  for (int i = 0; i < 100; ++i)
  {
    std::stringstream filename;
    filename << "frame-" << std::setfill('0') << std::setw(3) << i << ".jpeg";
    inFile.open(filename.str(), std::ios::in | std::ios::binary);
    inFile.ignore(std::numeric_limits<std::streamsize>::max());
    auto size = inFile.gcount();

    vtkNew<vtkCompressedVideoPacket> packet;
    packet->SetSize(size);
    auto dst = packet->GetData()->GetPointer(0);

    inFile.clear();
    inFile.seekg(0, std::ios_base::beg);
    inFile.read(reinterpret_cast<char*>(dst), size);

    auto status = dec->Push(packet);
    vtkLog(TRACE, << vtkVideoProcessingStatusTypeUtilities::ToString(status));
    auto result = dec->GetResult();
    vtkLog(TRACE, << vtkVideoProcessingStatusTypeUtilities::ToString(result.first));

    inFile.close();

    if (result.first != VTKVideoProcessingStatusType::VTKVPStatus_Success)
    {
      continue;
    }

    if (result.second.empty())
    {
      continue;
    }
    auto srcFrame = result.second.front();
    if (srcFrame != nullptr)
    {
      dstFrame->DeepCopy(srcFrame);
      dstFrame->Render(renWin);
    }
  }
  return 0;
}
