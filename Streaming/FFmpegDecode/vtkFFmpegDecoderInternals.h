/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFmpegDecoderInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkFFmpegDecoderInternals_h
#define vtkFFmpegDecoderInternals_h

#include "vtkType.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <chrono>
#include <string>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

class vtkCPUVideoFrame;

class vtkFFmpegDecoderInternals
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
  vtkMTimeType LastSetupMTime = 0;

  // timing for encode and scale operations.
  std::chrono::high_resolution_clock::time_point::duration dtDecode, dtScale;

  bool IsOutputFrameOutdated();
  bool InitializeOutputFrame(AVPixelFormat pixFmt);
  // returns a new raw video frame.
  vtkCPUVideoFrame* GetOutputFrameFromDecodedFrame();
  static VTKVideoProcessingStatusType ParseFFMPEGStatus(int statusCode, bool during_send = true);
};

#endif // vtkFFmpegDecoderInternals_h
// VTK-HeaderTest-Exclude: vtkFFmpegDecoderInternals.h
