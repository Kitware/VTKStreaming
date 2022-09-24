/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoDecoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVideoDecoder
 * @brief   this class defines an abstract interface for a video decoder.
 *
 * Similar to vtkVideoEncoder, the vtkVideoDecoder supports non-blocking decode.
 *
 * You can push compressed video packets with the vtkVideoDecoder::Push method.
 * Call vtkVideoDecoder::GetResult to obtain the uncompressed video frame.
 *
 * You are free to delete or modify the packet contents after calling `Push`.
 *
 * The decoder can use a processing delegate to queue packets into work units
 * which are eventually decoded.
 *
 * With both synchronous and asynchronous delegates, the 'real' decoding happens
 * during vtkVideoDecoder::GetResult().
 *
 * Here is an overview of the 3 important methods.
 *
 * With asynchronous delegate -:
 * 1. vtkVideoDecoder::Push() -
 *     does not block caller's thread. May start decoding when thread resources become available.
 * 2. vtkVideoDecoder::GetResult() -
 *     only attempts to get a result. May start decoding in worker thread if not already started.
 * 3. vtkVideoDecoder::HasResult() -
 *     returns true if any results are already available.
 *
 * You can avoid delegates if you prefer tighter control over the API. Ex -: implement your own task
 * queue management. Turn off async delegate with UseAsynchronousDelegateOff()
 * When the delegate is bypassed -:
 * 1. vtkVideoDecoder::Push() -
 *     blocks the caller's thread and decoding begins right away.
 * 2. vtkVideoDecoder::GetResult() -
 *     finishes the decoding and returns the result.
 * 3. vtkVideoDecoder::HasResult() -
 *     always returns false.
 *
 * With an asynchronous delegate, if you prefer to be notified
 * when a result is available, please listen to vtkCommand::ProgressEvent.
 * This class emits an event when frames are available.
 *
 * Call vtkVideoDecoder::Shutdown() before the decoder is destroyed.
 *
 * @sa vtkRawVideoFrame, vtkCompressedVideoPacket
 */

#ifndef vtkVideoDecoder_h
#define vtkVideoDecoder_h

#include "vtkObject.h"

#include "vtkPixelFormatTypes.h"             // for enum
#include "vtkStreamingDecodeModule.h"        // for export macro
#include "vtkVideoCodecTypes.h"              // for enum
#include "vtkVideoProcessingWorkUnitTypes.h" // for work unit

class vtkAsynchronousDecoderDelegate;
class vtkCompressedVideoPacket;

class VTKSTREAMINGDECODE_EXPORT vtkVideoDecoder : public vtkObject
{
public:
  vtkTypeMacro(vtkVideoDecoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get UseAsynchronousDelegate
   */
  void SetUseAsynchronousDelegate(bool val);
  bool GetUseAsynchronousDelegate();
  void UseAsynchronousDelegateOn();
  void UseAsynchronousDelegateOff();
  ///@}

  vtkSetEnumMacro(Codec, VTKVideoCodecType);
  vtkGetEnumMacro(Codec, VTKVideoCodecType);

  ///@{
  /**
   * Set the size of input buffers for the synchronous delegate.
   * Option has no effect for async delegate or when the delegates are bypassed.
   */
  vtkSetMacro(BufferSize, unsigned int);
  vtkGetMacro(BufferSize, unsigned int);
  ///@}

  ///@{
  /**
   * Public interface for the decoder. Concrete sub-classes are supposed to implement the
   * respective *Internal() methods to initialize and shutdown a decoding context.
   */
  bool Initialize();
  void Shutdown();
  void Flush();
  bool HasDelegate();
  ///@}

  ///@{
  /**
   * Public interface for the decoder. Concrete sub-classes are supposed to
   * implement the PushInternal() method.
   *
   * vtkVideoDecoder::Push(vtkCompressedVideoPacket* packet) is the entry point
   * to start decoding. The `Push` method is non-blocking.
   * The return value is false when any stage of the push
   * process failed.
   *
   * Call vtkVideoDecoder::GetResult() to access the decoded video frames.
   *
   * Draining the decoder is different from a flush operation.
   * Flush puts some decoder implementations in an uninitialized state whereas drain does not.
   */
  VTKVideoProcessingStatusType Push(vtkCompressedVideoPacket* packet);
  VTKVideoDecoderResultType Decode(vtkCompressedVideoPacket* packet);
  bool HasResult(); // always returns false when not using an asynchronous delegate.
  VTKVideoDecoderResultType GetResult();
  VTKVideoDecoderResultType Drain();
  ///@}

  /**
   * In asynchronous decoding, it may happen that a large number of frames are waiting in the task
   * queue. This method lets us ignore further decode requests when flushing the task queue.
   * vtkVideoDecoder::Push later resets the cancel flag.
   */
  void CancelPendingDecodeRequests();

  ///@{
  /**
   * Convenient functions implemented by concrete subclasses.
   */
  virtual bool IsHardwareAccelerated() const noexcept = 0;
  virtual bool SupportsAsynchronousDelegate() const noexcept = 0;
  virtual vtkIdType GetLastDecodeTimeNS() const noexcept = 0;
  virtual vtkIdType GetLastScaleTimeNS() const noexcept = 0;
  virtual bool SupportsCodec(VTKVideoCodecType codec) const noexcept = 0;
  ///@}

protected:
  vtkVideoDecoder();
  ~vtkVideoDecoder() override;

  VTKVideoCodecType Codec = VTKVideoCodecType::VTKVC_VP9;
  bool Initialized = false;
  bool IgnoreDecodeRequest = false;
  unsigned int BufferSize = 30;
  vtkAsynchronousDecoderDelegate* Delegate = nullptr;

  ///@{
  /**
   * Concrete subclasses must handle initialization, allocation and freeing of decoder resources.
   * The subclass must also translate the implementation error code to one of
   * VTKVideoProcessingStatusType.
   */
  virtual bool InitializeInternal() = 0;
  virtual void ShutdownInternal() = 0;
  virtual void FlushInternal() = 0;
  ///@

  ///@{
  /**
   * Concrete subclasses implement the process of decoding.
   */
  virtual VTKVideoProcessingStatusType PushInternal(vtkCompressedVideoPacket* packet) = 0;
  virtual VTKVideoDecoderResultType GetResultInternal() = 0;
  virtual VTKVideoDecoderResultType DecodeInternal(vtkCompressedVideoPacket* packet) = 0;
  virtual VTKVideoDecoderResultType DrainInternal() = 0;
  ///@}

private:
  vtkVideoDecoder(const vtkVideoDecoder&) = delete;
  void operator=(const vtkVideoDecoder&) = delete;
};

#endif // vtkVideoDecoder_h
