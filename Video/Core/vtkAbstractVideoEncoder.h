/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractVideoEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAbstractVideoEncoder
 * @brief   this class defines an abstract interface for a video encoder.
 *
 * You can push video frames for encoding with the vtkAbstractVideoEncoder::Push method.
 * Since the method is non-blocking, you have to call vtkAbstractVideoEncoder::GetResult
 * to obtain the compressed video packet.
 *
 * This class spawns one worker thread with `vtkThreadedTaskQueue` that picks up any queued frames
 * and encodes them without blocking the main thread. You are free to delete or modify the frame
 * contents after calling `Push`.
 *
 * In order to conservatively use memory, the encoder buffers frames to the task queue.
 * The size of the buffer is equal to the TimeBaseEnd value.
 *
 * @sa vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkAbstractVideoEncoder_h
#define vtkAbstractVideoEncoder_h

#include "vtkVideoCoreModule.h"

#include "vtkCodecTypes.h"
#include "vtkObject.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

class vtkRawVideoFrame;
class vtkAbstractEncoderDelegate;

class VTKVIDEOCORE_EXPORT vtkAbstractVideoEncoder : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractVideoEncoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void UseAsynchronousDelegate();
  void UseSynchronousDelegate(); // default

  ///@{
  /**
   * Set/Get codec used for video encoding.
   * Note: Subclasses implement vtkAbstractVideoEncoder::IsCodecSupported(VTKCodecType).
   * You can use that to set appropriate codec.
   */
  vtkSetEnumMacro(Codec, VTKCodecType);
  vtkGetEnumMacro(Codec, VTKCodecType);
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
  ///@}

  ///@{
  /**
   * Public interface for the encoder. Concrete sub-classes are supposed to
   * implement the PushInternal() method.
   *
   * vtkAbstractVideoEncoder::Push(vtkRawVideoFrame* frame) is the entry point
   * to start encoding. The `Push` method is non-blocking.
   * The return value is false when any stage of the push
   * process failed.
   *
   * Call vtkAbstractVideoEncoder::GetResult() to access the encoded video packets.
   */
  bool Push(vtkRawVideoFrame* frame);
  void Drain();
  VTKVideoProcessingStatusType GetResult(vtkSmartPointer<vtkCodedVideoPacket>& packet);
  ///@}

  ///@{
  /**
   * Convenient functions implemented by concrete subclasses.
   */
  virtual bool IsHardwareAccelerated() = 0;
  virtual vtkIdType GetLastEncodeTimeNS() = 0;
  virtual vtkIdType GetLastScaleTimeNS() = 0;
  virtual bool IsCodecSupported(VTKCodecType codec) = 0;
  ///@}

protected:
  vtkAbstractVideoEncoder();
  ~vtkAbstractVideoEncoder() override;

  // Codec context parameters.
  VTKCodecType Codec = VP9;
  // Sequence parameters
  bool ForceIFrame = false;
  int TimeBaseStart = 1;
  int TimeBaseEnd = 120;
  // Picture parameters.
  int GroupOfPicturesSize = 10;
  int MaximumBFrames = -1;
  // Bitrate control
  bool ForceCBR = true;           // sets MaxBitRate = MinBitRate = BitRate
  unsigned int BitRate = 1000000; // 1Mbps
  unsigned int MaxBitRate = 1000000;
  unsigned int MinBitRate = 1000000;
  // Parallelism
  unsigned int NumberOfEncoderThreads = 2; // conservative default.
  // Processing delegate
  vtkAbstractEncoderDelegate* Delegate = nullptr;

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
  virtual bool SetupEncoderFrame(const int& width, const int& height) = 0;
  virtual bool NeedsNewEncoderFrame(const int& width, const int& height) = 0;
  virtual void TearDownEncoderFrame() = 0;
  ///@}

  ///@{
  /**
   * Concrete subclasses will send a video frame for encoding and retrieve a compressed
   * video packet from the encoder.
   *
   * Warning: This method is called inside a worker thread. If your encoder context is not
   * thread-safe, please call that non-thread-safe functionaility outside this method.
   */
  virtual EncoderResultType EncodeInternal(vtkRawVideoFrame* frame) = 0;
  virtual void DrainInternal() = 0;
  ///@}

private:
  vtkAbstractVideoEncoder(const vtkAbstractVideoEncoder&) = delete;
  void operator=(const vtkAbstractVideoEncoder&) = delete;
};

#endif // vtkAbstractVideoEncoder_h
