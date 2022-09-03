/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGEncoderInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkFFMPEGEncoderInternals_h
#define vtkFFMPEGEncoderInternals_h

#include "vtkCodedVideoPacket.h"
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

class vtkAbstractEncoderDelegate;
class vtkRawVideoFrame;
class vtkCodedVideoPacket;

class vtkFFMPEGEncoderInternals
{
public:
  AVCodecContext* EncodeCtx = nullptr;
  AVFrame *SoftwareFrame = nullptr, *HardwareFrame = nullptr, *Frame = nullptr;
  AVPacket* Packet = nullptr;
  AVPixelFormat InputPixFmt;
  AVBufferRef* HardwareDevCtx = nullptr;

  const AVCodec* Codec = nullptr;
  struct SwsContext* SwScaleCtx = nullptr;

  int LastEncodedFrameDims[2] = { -1, -1 };

  std::string CodecName;
  // codec parameters are represented as key value pairs.
  std::unordered_map<std::string, std::string> CustomCodecParameters;

  // timing for encode and scale operations.
  std::chrono::high_resolution_clock::time_point::duration dtEncode, dtScale;

  bool InitializeHWEncodeCtx(AVHWDeviceType type);
  bool InitializeBasicEncodeCtx();

  bool InitializeCodec();
  bool SetupHWFrameCtx(AVPixelFormat HWPixelFormat);
  bool InitializeSWFrame();
  bool InitializeHWFrame();
  void Tweak();

  EncoderResultType Encode(bool keyFrame = false);
  bool PreprocessInput(vtkRawVideoFrame* frame);
  bool PrepareForEncoding();

  void Flush();
  void TearDownEncoderFrames();
  void Shutdown();

private:
  bool ConvertRGBA32ToEncoderPixFmt(vtkRawVideoFrame* rgba32Image);
  int Send(bool keyFrame = false);
  int Receive();
  vtkCodedVideoPacket* PackageCompressedPacket();
};

#endif // vtkFFMPEGEncoderInternals_h
