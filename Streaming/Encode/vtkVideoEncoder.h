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
 * @brief   defines an abstract interface for video encoding
 *
 * A video frame can be encoded with video codecs by following this process.
 *
 * 1. Setup your callback function with `SetOutputHandler`
 *
 * 2. Invoke `vtkVideoEncoder::Encode(vtkRawVideoFrame*).
 *
 * If, by limitation of language design, you're unable to install a callback,
 * please observe the vtkVideoEncoder::EncodedVideoChunkEvent for getting new chunks.
 *
 * @sa vtkRawVideoFrame, vtkCompressedVideoPacket
 */

#ifndef vtkVideoEncoder_h
#define vtkVideoEncoder_h

#include "vtkObject.h"

#include "vtkCommand.h"                      // for enum
#include "vtkPixelFormatTypes.h"             // for enum
#include "vtkRenderWindow.h"                 // for ivar
#include "vtkStreamingEncodeModule.h"        // for export macro
#include "vtkVideoCodecTypes.h"              // for enum
#include "vtkVideoProcessingStatusTypes.h"   // for enum
#include "vtkVideoProcessingWorkUnitTypes.h" // for work unit
#include "vtkWeakPointer.h"                  // for ivar

class vtkRawVideoFrame;

class VTKSTREAMINGENCODE_EXPORT vtkVideoEncoder : public vtkObject
{
public:
  vtkTypeMacro(vtkVideoEncoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  void SetOutputHandler(std::function<void(VTKVideoEncoderResultType)> outputHandler);

  enum
  {
    /**
     * Event fired when an encoded video chunk is available.
     */
    EncodedVideoChunkEvent = vtkCommand::UserEvent + 1
  };

  ///@{
  /**
   * Public interface for the encoder. Concrete sub-classes are supposed to implement the
   * respective *Internal() methods to initialize and shutdown an encoding context.
   */
  bool Initialize();
  void Shutdown();
  ///@}

  ///@{
  /**
   * Flush asks the encoder for any remaining packets
   * Drain is similar to flush but does not send an EOS to end the stream.
   */
  void Flush();
  void Drain();
  ///@}

  ///@{
  /**
   * Public interface for the encoder. Concrete sub-classes are supposed to
   * implement EncodeInternal(vtkSmartPointer<vtkRawVideoFrame>), EncodeInternal().
   *
   */
  void Encode(vtkSmartPointer<vtkRawVideoFrame> frame);
  ///@}

  ///@{
  /**
   * Convenient functions implemented by concrete subclasses.
   */
  virtual bool IsHardwareAccelerated() const noexcept = 0;
  virtual vtkIdType GetLastEncodeTimeNS() const noexcept = 0;
  virtual vtkIdType GetLastScaleTimeNS() const noexcept = 0;
  virtual bool SupportsCodec(VTKVideoCodecType codec) const noexcept = 0;
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
   * Set/Get a graphics context. Some hardware encoders
   * may need it to initialize frames based on that graphics context.
   * When the encoder is in async mode, it maintains a thread-local
   * context. You can access it with `GetDelegateContext()`
   */
  void SetGraphicsContext(vtkRenderWindow* context);
  vtkRenderWindow* GetGraphicsContext() const;
  ///@}

  enum class BRCType
  {
    CBR, // MaxBitRate = MinBitRate = BitRate
    VBR, // Bitrate may fluctuate within set minimum and maximum.
    CQP  // no bitrate control. Set QuantizationParameter
  };

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
  int TimeBaseEnd = 30;
  int GroupOfPicturesSize = 10;
  int MaximumBFrames = -1;
  // 3. Picture parameters.
  int Width = 240;
  int Height = 240;
  VTKPixelFormatType InputPixelFormat = VTKPixelFormatType::VTKPF_NV12;
  // 4. Bitrate control (in bits per second)
  BRCType BitRateControlMode = BRCType::CBR;
  unsigned int QuantizationParameter = 33;
  unsigned int BitRate = 1000000; // 1Mbps
  unsigned int MaxBitRate = 1000000;
  unsigned int MinBitRate = 1000000;
  // 5. Parallelism
  unsigned int NumberOfEncoderThreads = 2; // conservative default.
  // 6. Our graphics context.
  vtkWeakPointer<vtkRenderWindow> GraphicsContext;
  // 7. handler
  std::function<void(VTKVideoEncoderResultType)> OutputHandler = nullptr;
  // 8. state
  bool Initialized = false;
  vtkMTimeType LastSetupMTime = 0;

  /**
   * Returns true if given width and height do not match the encoding context's width and height.
   */
  bool NeedsNewEncoderFrame(int width, int height);

  ///@{
  /**
   * Concrete subclasses must handle initialization, allocation and freeing of encoder resources.
   */
  virtual bool InitializeInternal() = 0;
  virtual void ShutdownInternal() = 0;
  ///@}

  ///@{
  /**
   * Concrete subclasses must handle initialization, allocation and freeing of an encoder frame
   * resource.
   */
  virtual bool SetupEncoderFrame(int width, int height) = 0;
  virtual void TearDownEncoderFrame() = 0;
  ///@}

  ///@{
  /**
   * Concrete subclasses send a frame for encoding and return the output.
   */
  virtual VTKVideoEncoderResultType EncodeInternal(vtkSmartPointer<vtkRawVideoFrame> frame) = 0;
  virtual VTKVideoEncoderResultType SendEOS() = 0;
  ///@}

private:
  vtkVideoEncoder(const vtkVideoEncoder&) = delete;
  void operator=(const vtkVideoEncoder&) = delete;

  VTKVideoProcessingStatusType UpdateEncoderContext(int width, int height);
  vtkSmartPointer<vtkRawVideoFrame> PrepareThreadLocalResources(
    vtkSmartPointer<vtkRawVideoFrame> from);
};

#endif // vtkVideoEncoder_h
