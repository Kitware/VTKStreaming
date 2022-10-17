/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFmpegEncoderInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkFFmpegEncoderInternals_h
#define vtkFFmpegEncoderInternals_h

#include "vtkCompressedVideoPacket.h"
#include "vtkRawVideoFrame.h"
#include "vtkSmartPointer.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <chrono>
#include <string>
#include <unordered_map>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/buffer.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

class vtkEncoderDelegate;
class vtkRawVideoFrame;
class vtkCompressedVideoPacket;

class vtkFFmpegEncoderInternals
{
public:
  AVCodecContext* EncodeCtx = nullptr;
  AVFrame *SoftwareFrame = nullptr, *HardwareFrame = nullptr, *Frame = nullptr;
  AVPacket* Packet = nullptr;
  AVPixelFormat InputPixFmt;
  AVBufferRef* HardwareDevCtx = nullptr;

  const AVCodec* Codec = nullptr;
  struct SwsContext* SwScaleCtx = nullptr;

  uint64_t SendCounter = 0;

  std::string CodecName;
  // codec parameters are represented as key value pairs.
  std::unordered_map<std::string, std::string> CustomCodecParameters;

  // timing for encode and scale operations.
  std::chrono::high_resolution_clock::time_point::duration dtEncode, dtScale;
  std::chrono::high_resolution_clock::time_point t1, t2;

  bool InitializeHWEncodeCtx(AVHWDeviceType type);
  bool InitializeBasicEncodeCtx();

  bool InitializeCodec();
  bool SetupHWFrameCtx(AVPixelFormat HWPixelFormat);
  bool InitializeSWFrame();
  bool InitializeHWFrame();
  void Tweak();

  bool PreprocessInput(vtkRawVideoFrame* frame);
  bool PrepareForEncoding();
  int Send(bool keyFrame = false);
  int Receive();
  VTKVideoEncoderResultType Encode(bool keyFrame = false);

  void Flush();
  void TearDownEncoderFrames();
  void Shutdown();

  vtkCompressedVideoPacket* PackageCompressedPacket();
  static VTKVideoProcessingStatusType ParseFFMPEGStatus(int statusCode, bool during_send = true);

private:
  bool ConvertRGBA32ToEncoderPixFmt(vtkRawVideoFrame* rgba32Image);
};

#endif // vtkFFmpegEncoderInternals_h
// VTK-HeaderTest-Exclude: vtkFFmpegEncoderInternals.h
