/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractVideoDecoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAbstractVideoDecoder
 * @brief   this class defines an abstract interface for a video decoder.
 *
 * Similar to vtkAbstractVideoEncoder, this class permits non-blocking decode.
 *
 * You can push video packets for decoding with the vtkAbstractVideoDecoder::Push method.
 * Since the method is non-blocking, you have to call vtkAbstractVideoDecoder::GetResult
 * to obtain the uncompressed video frame.
 *
 * This class spawns one worker thread with `vtkThreadedTaskQueue` that picks up any queued packets
 * and decodes them without blocking the main thread. You are free to delete or modify the packet
 * contents after calling `Push`.
 *
 * @sa vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkAbstractVideoDecoder_h
#define vtkAbstractVideoDecoder_h

#include "vtkVideoCoreModule.h"

#include "vtkCodecTypes.h"
#include "vtkObject.h"
#include "vtkPixelFormats.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

class vtkCodedVideoPacket;
class vtkAbstractDecoderDelegate;

class VTKVIDEOCORE_EXPORT vtkAbstractVideoDecoder : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractVideoDecoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void UseAsynchronousDelegate();
  void UseSynchronousDelegate(); // default

  vtkSetEnumMacro(CodecType, VTKCodecType);
  vtkGetEnumMacro(CodecType, VTKCodecType);

  vtkSetEnumMacro(OutputPixelFormat, VTKPixelFormat);
  vtkGetEnumMacro(OutputPixelFormat, VTKPixelFormat);

  vtkSetMacro(BufferSize, unsigned int);
  vtkGetMacro(BufferSize, unsigned int);

  ///@{
  /**
   * Public interface for the decoder. Concrete sub-classes are supposed to implement the
   * respective *Internal() methods to initialize and shutdown a decoding context.
   */
  bool Initialize();
  void Shutdown();
  void Flush();
  ///@}

  ///@{
  /**
   * Public interface for the decoder. Concrete sub-classes are supposed to
   * implement the PushInternal() method.
   *
   * vtkAbstractVideoDecoder::Push(vtkRawVideoFrame* frame) is the entry point
   * to start decoding. The `Push` method is non-blocking.
   * The return value is false when any stage of the push
   * process failed.
   *
   * Call vtkAbstractVideoDecoder::GetResult() to access the decoded video frames.
   */
  bool Push(vtkCodedVideoPacket* packet);
  void Drain();
  VTKVideoProcessingStatusType GetResult(vtkSmartPointer<vtkRawVideoFrame>& packet);
  ///@}

  ///@{
  /**
   * Convenient functions implemented by concrete subclasses.
   */
  virtual bool IsHardwareAccelerated() = 0;
  virtual vtkIdType GetLastDecodeTimeNS() = 0;
  virtual vtkIdType GetLastScaleTimeNS() = 0;
  ///@}

protected:
  vtkAbstractVideoDecoder();
  ~vtkAbstractVideoDecoder() override;

  VTKCodecType CodecType = VP9;
  VTKPixelFormat OutputPixelFormat = RGBA32;
  bool Initialized = false;
  unsigned int BufferSize = 30;
  vtkAbstractDecoderDelegate* Delegate = nullptr;

  ///@{
  /**
   * Concrete subclasses must handle initialization, allocation and freeing of decoder resources.
   * The subclass must also translate the implementation error code to one of VTKVideoProcessingStatusType.
   */
  virtual bool InitializeInternal() = 0;
  virtual void ShutdownInternal() = 0;
  virtual void FlushInternal() = 0;
  ///@

  ///@{
  /**
   * Concrete subclasses implement the process of decoding.
   */
  virtual void DrainInternal() = 0;
  virtual DecoderResultType DecodeInternal(vtkCodedVideoPacket* frame) = 0;
  ///@}

private:
  vtkAbstractVideoDecoder(const vtkAbstractVideoDecoder&) = delete;
  void operator=(const vtkAbstractVideoDecoder&) = delete;
};

#endif // vtkAbstractVideoDecoder_h
