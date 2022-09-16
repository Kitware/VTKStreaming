/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVideoEncoder
 * @brief   this class defines an abstract interface for a video encoder.
 *
 * You can push video frames for encoding with the vtkVideoEncoder::Push method.
 * Since the method is non-blocking, you have to call vtkVideoEncoder::GetResult
 * to obtain the compressed video packet.
 *
 * You are free to delete or modify the frame contents after calling `Push`.
 *
 * The encoder can use a processing delegate to queue frames into work units
 * which are eventually encoded.
 *
 * With both synchronous and asynchronous delegates, the 'real' encoding happens
 * during vtkVideoEncoder::GetResult().
 *
 * Here is an overview of the 3 important methods with different delegates.
 *
 * With synchronous delegate -:
 * 1. vtkVideoEncoder::Push() -
 *     does not block caller's thread.
 * 2. vtkVideoEncoder::GetResult() -
 *     starts and finishes encoding on the caller's thread.
 * 3. vtkVideoEncoder::HasResult() -
 *     always returns false.
 *
 * With asynchronous delegate -:
 * 1. vtkVideoEncoder::Push() -
 *     does not block caller's thread. May start encoding when thread resources become available.
 * 2. vtkVideoEncoder::GetResult() -
 *     only attempts to get a result. May start encoding in worker thread if not already started.
 * 3. vtkVideoEncoder::HasResult() -
 *     returns true if any results are already available.
 *
 * You can avoid delegates if you prefer tighter control over the API. Ex -: implement your own task
 * queue management.
 * When the delegate is bypassed -:
 * 1. vtkVideoEncoder::Push() -
 *     blocks the caller's thread and encoding begins right away.
 * 2. vtkVideoEncoder::GetResult() -
 *     finishes the encoding and returns the result.
 * 3. vtkVideoEncoder::HasResult() -
 *     always returns false.
 *
 * @sa vtkRawVideoFrame, vtkCompressedVideoPacket
 */

#ifndef vtkVideoEncoder_h
#define vtkVideoEncoder_h

#include "vtkObject.h"

#include "vtkPixelFormatTypes.h"             // for enum
#include "vtkStreamingEncodeModule.h"        // for export macro
#include "vtkVideoCodecTypes.h"              // for enum
#include "vtkVideoProcessingStatusTypes.h"   // for enum
#include "vtkVideoProcessingWorkUnitTypes.h" // for work unit

class vtkRawVideoFrame;
class vtkEncoderDelegate;

class VTKSTREAMINGENCODE_EXPORT vtkVideoEncoder : public vtkObject
{
public:
  vtkTypeMacro(vtkVideoEncoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Switch between different delegates or bypass the delegates.
   */
  void UseAsynchronousDelegate();
  void UseSynchronousDelegate(); // default
  void SetBypassDelegate(bool val);
  bool GetBypassDelegate();
  void BypassDelegateOn();
  void BypassDelegateOff();
  ///@}

  ///@{
  /**
   * Requests the encoder to return packets as soon as possible without
   * buffering frames internally.
   */
  void SetForceLowLatency(bool val);
  bool GetForceLowLatency();
  void ForceLowLatencyOn();
  void ForceLowLatencyOff();
  ///@}

  ///@{
  /**
   * Requests the encoder to return packets as soon as possible without
   * buffering frames internally.
   */
  void SetKeyFramesOnly(bool val);
  bool GetKeyFramesOnly();
  void KeyFramesOnlyOn();
  void KeyFramesOnlyOff();
  ///@}

  ///@{
  /**
   * Set/Get codec used for video encoding.
   */
  vtkSetEnumMacro(Codec, VTKVideoCodecType);
  vtkGetEnumMacro(Codec, VTKVideoCodecType);
  ///@}

  ///@{
  /**
   * Set/Get width and height of the pictures.
   */
  vtkSetMacro(Width, int);
  vtkGetMacro(Width, int);
  vtkSetMacro(Height, int);
  vtkGetMacro(Height, int);
  ///@}

  ///@{
  /**
   * Set/Get pixel format of the input pictures.
   */
  vtkSetEnumMacro(InputPixelFormat, VTKPixelFormatType);
  vtkGetEnumMacro(InputPixelFormat, VTKPixelFormatType);
  ///@}

  ///@{
  /**
   * Set/Get size of group of pictures.
   * A group of pictures is made up of many P-frames. (predictive frames)
   * Smaller values yield higher quality.
   * A larger value translates to poor quality but faster encode times.
   */
  vtkSetMacro(GroupOfPicturesSize, int);
  vtkGetMacro(GroupOfPicturesSize, int);
  ///@}

  ///@{
  /**
   * Set/Get maximum number of bi-directional frames.
   * This feature is functional only if the codec and encoder support bi-directional prediction.
   */
  vtkSetMacro(MaximumBFrames, int);
  vtkGetMacro(MaximumBFrames, int);
  ///@}

  ///@{
  /**
   * Set/Get start time step. This is usually 1-index based.
   */
  vtkSetMacro(TimeBaseStart, int);
  vtkGetMacro(TimeBaseStart, int);
  ///@}

  ///@{
  /**
   * Set/Get end time step. This is usually the frames-per-second of the encoder.
   */
  vtkSetMacro(TimeBaseEnd, int);
  vtkGetMacro(TimeBaseEnd, int);
  ///@}

  ///@{
  /**
   * Set/Get bitrate of an encoder.
   * Note: This value is used only when the encoder and codec support rate-control
   * AND they are configured in CBR (Constant Bit Rate) mode.
   */
  vtkSetMacro(BitRate, unsigned int);
  vtkGetMacro(BitRate, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get maximum bitrate of an encoder.
   * Note: This value is used only when the encoder and codec support rate-control
   * AND they are configured in VBR (Variable Bit Rate) mode.
   */
  vtkSetMacro(MaxBitRate, unsigned int);
  vtkGetMacro(MaxBitRate, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get minimum bitrate of an encoder.
   * Note: This value is used only when the encoder and codec support rate-control
   * AND they are configured in VBR (Variable Bit Rate) mode.
   */
  vtkSetMacro(MinBitRate, unsigned int);
  vtkGetMacro(MinBitRate, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get number of threads the encoder must use.
   * Almost every encoder out there is multi-threaded and typically supports
   * tiling up video frames into rows and columns. Use it wisely.
   */
  vtkSetMacro(NumberOfEncoderThreads, unsigned int);
  vtkGetMacro(NumberOfEncoderThreads, unsigned int);
  ///@}

  ///@{
  /**
   * The encoder can force a KeyFrame i.e, an I-Frame irrelevant of
   * the GOP size.
   *
   * DevNote: All these should not update MTime. Otherwise, an encoder will reinitialize its
   * context.
   */
  void SetForceIFrame(bool val);
  bool GetForceIFrame();
  void ForceIFrameOn();
  void ForceIFrameOff();
  ///@}

  ///@{
  /**
   * Force the encoder into CBR(Constant Bit Rate) mode.
   *
   * DevNote: All these should try their best to put the encoder into CBR mode.
   * The abstract class simply sets MaxBitRate = MinBitRate = BitRate. Usually,
   * this is sufficient.
   */
  virtual void SetForceCBR(bool val);
  bool GetForceCBR();
  void ForceCBROn();
  void ForceCBROff();
  ///@}

  ///@{
  /**
   * Public interface for the encoder. Concrete sub-classes are supposed to implement the
   * respective *Internal() methods to initialize and shutdown an encoding context.
   */
  bool Initialize();
  void Shutdown();
  void Flush();
  bool HasDelegate();
  ///@}

  ///@{
  /**
   * Public interface for the encoder. Concrete sub-classes are supposed to
   * implement the PushInternal(), GetResultInternal() and EncodeInternal() methods.
   *
   * vtkVideoEncoder::Push(vtkRawVideoFrame* frame) is the entry point
   * to start encoding.
   *
   * Call vtkVideoEncoder::GetResult() to access the encoded video packets.
   */
  VTKVideoEncoderResultType Drain();
  VTKVideoProcessingStatusType Push(vtkRawVideoFrame* frame);
  bool HasResult(); // always returns false when not using an asynchronous delegate.
  VTKVideoEncoderResultType GetResult();
  ///@}

  ///@{
  /**
   * Convenient functions implemented by concrete subclasses.
   */
  virtual bool IsHardwareAccelerated() const noexcept = 0;
  virtual bool SupportsAsynchronousDelegate() const noexcept = 0;
  virtual bool SupportsSynchronousDelegate() const noexcept = 0;
  virtual vtkIdType GetLastEncodeTimeNS() const noexcept = 0;
  virtual vtkIdType GetLastScaleTimeNS() const noexcept = 0;
  virtual bool SupportsCodec(VTKVideoCodecType codec) const noexcept = 0;
  ///@}

protected:
  vtkVideoEncoder();
  ~vtkVideoEncoder() override;

  // 1. Codec context parameters.
  VTKVideoCodecType Codec = VTKVideoCodecType::VTKVC_VP9;
  bool ForceLowLatency = true;
  bool KeyFramesOnly = false;
  // 2. Sequence parameters
  bool ForceIFrame = false;
  int TimeBaseStart = 1;
  int TimeBaseEnd = 120;
  int GroupOfPicturesSize = 10;
  int MaximumBFrames = -1;
  // 3. Picture parameters.
  int Width = 240;
  int Height = 240;
  VTKPixelFormatType InputPixelFormat = VTKPixelFormatType::VTKPF_NV12;
  // 4. Bitrate control
  bool ForceCBR = true;           // sets MaxBitRate = MinBitRate = BitRate
  unsigned int BitRate = 1000000; // 1Mbps
  unsigned int MaxBitRate = 1000000;
  unsigned int MinBitRate = 1000000;
  // 5. Parallelism
  unsigned int NumberOfEncoderThreads = 2; // conservative default.
  // 6. Processing delegate
  vtkEncoderDelegate* Delegate = nullptr;
  bool BypassDelegate = false;
  bool TryIndefinitelyUntilReceiveValidPacket = false;

  bool Initialized = false;
  vtkMTimeType LastSetupMTime = 0;

  ///@{
  /**
   * Concrete subclasses must handle initialization, allocation and freeing of encoder resources.
   */
  virtual bool InitializeInternal() = 0;
  virtual void ShutdownInternal() = 0;
  virtual void FlushInternal() = 0;
  ///@}

  ///@{
  /**
   * Concrete subclasses must handle initialization, allocation and freeing of an encoder frame
   * resource.
   */
  virtual bool SetupEncoderFrame(int width, int height) = 0;
  virtual bool NeedsNewEncoderFrame(int width, int height) = 0;
  virtual void TearDownEncoderFrame() = 0;
  ///@}

  ///@{
  /**
   * Concrete subclasses will send a video frame for encoding and retrieve a compressed
   * video packet from the encoder.
   *
   * PushInternal and GetResultInternal are invoked when we bypass the delegate.
   *
   * Warning: EncodeInternal is invoked inside a worker thread. If your encoder context is not
   * thread-safe, please call that non-thread-safe functionaility outside this method.
   *
   */
  virtual VTKVideoProcessingStatusType PushInternal(vtkRawVideoFrame* frame) = 0;
  virtual VTKVideoEncoderResultType GetResultInternal() = 0;
  virtual VTKVideoEncoderResultType EncodeInternal(vtkRawVideoFrame* frame) = 0;
  virtual VTKVideoEncoderResultType DrainInternal() = 0;
  ///@}

private:
  vtkVideoEncoder(const vtkVideoEncoder&) = delete;
  void operator=(const vtkVideoEncoder&) = delete;
};

#endif // vtkVideoEncoder_h
