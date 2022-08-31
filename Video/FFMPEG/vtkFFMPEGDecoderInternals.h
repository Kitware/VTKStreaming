/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGDecoderInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkFFMPEGDecoderInternals_h
#define vtkFFMPEGDecoderInternals_h

#include "vtkRawVideoFrame.h"
#include "vtkType.h"

#include <chrono>
#include <string>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

class vtkFFMPEGDecoderInternals
{
public:
  // data
  AVCodecContext* DecodeCtx;
  AVFrame* SoftwareFrame = nullptr;
  AVPacket* Packet;
  AVPixelFormat OutputPixFmt;

  const AVCodec* Codec = nullptr;
  struct SwsContext* SwScaleCtx = nullptr;

  std::string CodecName;
  vtkMTimeType LastSetupMTime = -1;

  // timing for encode and scale operations.
  std::chrono::high_resolution_clock::time_point::duration dtDecode, dtScale;
  vtkNew<vtkRawVideoFrame> OutputVideoFrame;

  bool IsOutputFrameOutdated();
  bool InitializeOutputFrame(AVPixelFormat pixFmt);
  bool GetOutputFrameFromDecodedFrame();
};

#endif // vtkFFMPEGDecoderInternals_h
