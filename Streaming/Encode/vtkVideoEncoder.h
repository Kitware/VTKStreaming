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
 * There are two ways to achieve video encoding with this class.
 *
 * 1. You can push `vtkRawVideoFrame` objects for encoding with the
 *    vtkVideoEncoder::Push(vtkRawVideoFrame*) method.
 *    Since the method may be non-blocking, you need to call
 *    vtkVideoEncoder::GetResult() to obtain the compressed video packet.
 *    Async mode is supported.
 *
 * 2. When your use case involves streaming a display, usually from `vtkRenderWindow`,
 *    you can achieve zero-copy with certain hardware accelerated video encoders this way.
 *
 * Supply the `vtkRenderWindow` instance with
 * vtkVideoEncoder::SetGraphicsContext(vtkRenderWindow*). Then, call
 * vtkVideoEncoder::EncodeDisplay(vtkRenderWindow*) whenever you're ready.
 * The return value will have chunks of encoded video corresponding to
 * `vtkRenderWindow` display frame buffer. This interesting use case is for
 * low-latency live encoding with software/hardware acclerated encoders.
 * Async mode is not supported.
 *
 * You are free to delete or modify the frame contents after calling `Push`,
 * only when using asynchronous delegate.
 *
 * In async mode, the encoder can use an asynchronous delegate to
 * queue frames into work units which are eventually encoded.
 *
 * With asynchronous delegates, the 'real' encoding happens
 * during vtkVideoEncoder::GetResult().
 *
 * Here is an overview of the 3 important methods.
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
 * queue management. Turn off async delegate with AsyncModeOff()
 * When the delegate is bypassed -:
 * 1. vtkVideoEncoder::Push() -
 *     blocks the caller's thread and encoding begins right away.
 * 2. vtkVideoEncoder::GetResult() -
 *     finishes the encoding and returns the result.
 * 3. vtkVideoEncoder::HasResult() -
 *     always returns false.
 *
 * With an asynchronous delegate, if you prefer to be notified
 * when a result is available, please listen to vtkCommand::ProgressEvent.
 * This class emits an event when packets are available.
 *
 * Call vtkVideoEncoder::Shutdown() before the encoder is destroyed.
 *
 * @sa vtkRawVideoFrame, vtkCompressedVideoPacket
 */

#ifndef vtkVideoEncoder_h
#define vtkVideoEncoder_h

#include "vtkObject.h"

#include "vtkPixelFormatTypes.h"             // for enum
#include "vtkRenderWindow.h"                 // for ivar
#include "vtkStreamingEncodeModule.h"        // for export macro
#include "vtkVideoCodecTypes.h"              // for enum
#include "vtkVideoProcessingStatusTypes.h"   // for enum
#include "vtkVideoProcessingWorkUnitTypes.h" // for work unit
#include "vtkWeakPointer.h"                  // for ivar

class vtkAsynchronousEncoderDelegate;
class vtkRawVideoFrame;

class VTKSTREAMINGENCODE_EXPORT vtkVideoEncoder : public vtkObject
{
public:
  vtkTypeMacro(vtkVideoEncoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum class BRCType
  {
    CBR, // MaxBitRate = MinBitRate = BitRate
    VBR, // Bitrate may fluctuate within set minimum and maximum.
    CQP  // no bitrate control. Set QuantizationParameter
  };

  ///@{
  /**
   * Set/Get a graphics context. Some hardware encoders
   * may need it to initialize frames based on that graphics context.
   * When the encoder is in async mode, it maintains a thread-local
   * context. You can access it with `GetDelegateContext()`
   */
  void SetGraphicsContext(vtkRenderWindow* context);
  vtkRenderWindow* GetGraphicsContext() const;
  vtkRenderWindow* GetDelegateGraphicsContext() const;
  ///@}

  ///@{
  /**
   * Set/Get async mode
   */
  void SetAsyncMode(bool val);
  bool GetAsyncMode();
  void AsyncModeOn();
  void AsyncModeOff();
  ///@}

  ///@{
  /**
   * Requests the encoder to return packets as soon as possible without
   * buffering frames internally.
   */
  vtkSetMacro(LowDelayMode, bool);
  vtkGetMacro(LowDelayMode, bool);
  vtkBooleanMacro(LowDelayMode, bool);
  ///@}

  ///@{
  /**
   * Requests the encoder to return packets as soon as possible without
   * buffering frames internally.
   */
  vtkSetMacro(KeyFramesOnly, bool);
  vtkGetMacro(KeyFramesOnly, bool);
  vtkBooleanMacro(KeyFramesOnly, bool);
  ///@}

  ///@{
  /**
   * Set/Get codec used for video encoding.
   */
  vtkSetEnumMacro(Codec, VTKVideoCodecType);
  vtkGetEnumMacro(Codec, VTKVideoCodecType);
  void SetCodec(int codec);
  ///@}

  ///@{
  /**
   * Set/Get width and height of encoding context.
   */
  void SetWidth(int width);
  vtkGetMacro(Width, int);
  void SetHeight(int height);
  vtkGetMacro(Height, int);
  ///@}

  ///@{
  /**
   * Set/Get pixel format of the input pictures.
   */
  vtkSetEnumMacro(InputPixelFormat, VTKPixelFormatType);
  vtkGetEnumMacro(InputPixelFormat, VTKPixelFormatType);
  void SetInputPixelFormat(int pixFmt);
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
   * Set/Get bitrate control mode of an encoder.
   */
  vtkSetEnumMacro(BitRateControlMode, BRCType);
  vtkGetEnumMacro(BitRateControlMode, BRCType);
  void SetBitRateControlMode(int mode);
  ///@}

  ///@{
  /**
   * Set/Get quantization parameter of an encoder.
   * Note: This value is used only when the encoder and codec support rate-control
   * AND they are configured in CQP (Constant Quantization Parameter) mode.
   */
  vtkSetClampMacro(QuantizationParameter, unsigned int, 1, 60);
  vtkGetMacro(QuantizationParameter, unsigned int);
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
   * Public interface for the encoder. Concrete sub-classes are supposed to implement the
   * respective *Internal() methods to initialize and shutdown an encoding context.
   */
  bool Initialize();
  void Shutdown();
  bool HasDelegate();
  ///@}

  ///@{
  /**
   * Draining the encoder is different from a flush operation in two ways.
   * - Flush puts some encoder implementations in an uninitialized state whereas drain does not.
   * - Drain asks the encoder for any remaining packets and gives them to you. Flush doesn't care to
   * do that.
   */
  void Flush();
  VTKVideoEncoderResultType Drain();
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
   *
   */
  VTKVideoProcessingStatusType Push(vtkRawVideoFrame* frame);
  VTKVideoEncoderResultType Encode(VTKVideoEncoderInputType frame);
  VTKVideoEncoderResultType GetResult();
  bool HasResult(); // always returns false when not using an asynchronous delegate.
  ///@}

  ///@{
  /**
   * Capture the display from current graphics context and encode the image.
   * Some synchronous hardware encoders can do zero-copy encoding.
   * Hardware encoders are usually fast enough to be non-blocking.
   */
  VTKVideoEncoderResultType EncodeDisplay();
  ///@}

  /**
   * In asynchronous encoding, it may happen that a large number of frames are waiting in the task
   * queue. This method lets us ignore further encode requests when flushing the task queue.
   * vtkVideoEncoder::Push resets the cancel flag.
   */
  void CancelPendingEncodeRequests();

  ///@{
  /**
   * Convenient functions implemented by concrete subclasses.
   */
  virtual bool IsHardwareAccelerated() const noexcept = 0;
  virtual bool SupportsAsyncMode() const noexcept = 0;
  virtual bool SupportsZeroCopy() const noexcept = 0;
  virtual vtkIdType GetLastEncodeTimeNS() const noexcept = 0;
  virtual vtkIdType GetLastScaleTimeNS() const noexcept = 0;
  virtual bool SupportsCodec(VTKVideoCodecType codec) const noexcept = 0;
  ///@}

protected:
  vtkVideoEncoder();
  ~vtkVideoEncoder() override;

  // 1. Codec context parameters.
  VTKVideoCodecType Codec = VTKVideoCodecType::VTKVC_VP9;
  bool LowDelayMode = true;
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
  BRCType BitRateControlMode = BRCType::CBR;
  unsigned int QuantizationParameter = 33;
  unsigned int BitRate = 1000000; // 1Mbps
  unsigned int MaxBitRate = 1000000;
  unsigned int MinBitRate = 1000000;
  // 5. Parallelism
  unsigned int NumberOfEncoderThreads = 2; // conservative default.
  // 6. Processing delegate
  vtkAsynchronousEncoderDelegate* Delegate = nullptr;
  // 7. Our graphics context.
  vtkWeakPointer<vtkRenderWindow> GraphicsContext;
  bool DirectDisplayEncodeMode = false;

  bool Initialized = false;
  bool IgnoreEncodeRequest = false;
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
  bool NeedsNewEncoderFrame(int width, int height);
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
  virtual VTKVideoProcessingStatusType PushInternal(VTKVideoEncoderInputType frame) = 0;
  virtual VTKVideoEncoderResultType GetResultInternal() = 0;
  virtual VTKVideoEncoderResultType EncodeInternal(VTKVideoEncoderInputType frame) = 0;
  virtual VTKVideoEncoderResultType DrainInternal() = 0;
  virtual VTKVideoEncoderResultType EncodeDisplayInternal() = 0;
  ///@}

private:
  vtkVideoEncoder(const vtkVideoEncoder&) = delete;
  void operator=(const vtkVideoEncoder&) = delete;

  VTKVideoProcessingStatusType UpdateEncoderContext(int width, int height);
};

#endif // vtkVideoEncoder_h
