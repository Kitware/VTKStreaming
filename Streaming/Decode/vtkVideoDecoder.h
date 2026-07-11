// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
/**
 * @class   vtkVideoDecoder
 * @brief   defines an abstract interface for video decoding
 *
 * An encoded video chunk can be decoded with video codecs by following this process.
 *
 * 1. Setup your callback function with `SetOutputHandler`
 *
 * 2. Invoke `vtkVideoDecoder::Decode(vtkCompressedVideoPacket*).
 *
 * If, by limitation of language design, you're unable to install a callback,
 * please observe the `vtkVideoDecoder::DecodedVideoFrameEvent` for getting new frames.
 *
 * @sa vtkRawVideoFrame, vtkCompressedVideoPacket
 */

#ifndef vtkVideoDecoder_h
#define vtkVideoDecoder_h

#include "vtkObject.h"

#include "vtkCommand.h"                      // for enum
#include "vtkStreamingDecodeModule.h"        // for export macro
#include "vtkVideoCodecTypes.h"              // for enum
#include "vtkVideoProcessingWorkUnitTypes.h" // for work unit

class vtkCompressedVideoPacket;
class vtkRenderWindow;

class VTKSTREAMINGDECODE_EXPORT vtkVideoDecoder : public vtkObject
{
public:
  vtkTypeMacro(vtkVideoDecoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    /**
     * Event fired when a decoded frame is available.
     */
    DecodedVideoFrameEvent = vtkCommand::UserEvent + 1
  };

  vtkSetEnumMacro(Codec, VTKVideoCodecType);
  vtkGetEnumMacro(Codec, VTKVideoCodecType);

  ///@{
  /**
   * Set/Get a graphics context.
   */
  void SetGraphicsContext(vtkRenderWindow* context);
  vtkRenderWindow* GetGraphicsContext() const;
  ///@}
  void SetOutputHandler(std::function<void(VTKVideoDecoderResultType)> outputHandler);

  ///@{
  /**
   * Public interface for the decoder. Concrete sub-classes are supposed to implement the
   * respective *Internal() methods to initialize and shutdown an encoding context.
   */
  bool Initialize();
  void Shutdown();
  ///@}

  ///@{
  /**
   * Flush asks the decoder for any remaining packets
   * Drain is similar to flush but does not send an EOS to end the stream.
   */
  void Flush();
  void Drain();
  ///@}

  ///@{
  /**
   * Public interface for the decoder. Concrete sub-classes are supposed to
   * implement DecodeInternal(vtkSmartPointer<vtkCompressedVideoPacket>), DecodeInternal().
   *
   */
  void Decode(vtkSmartPointer<vtkCompressedVideoPacket> frame);
  ///@}

  ///@{
  /**
   * Convenient functions implemented by concrete subclasses.
   */
  virtual bool IsHardwareAccelerated() const noexcept = 0;
  virtual vtkIdType GetLastDecodeTimeNS() const noexcept = 0;
  virtual vtkIdType GetLastScaleTimeNS() const noexcept = 0;
  virtual bool SupportsCodec(VTKVideoCodecType codec) const noexcept = 0;
  ///@}

protected:
  vtkVideoDecoder();
  ~vtkVideoDecoder() override;

  VTKVideoCodecType Codec = VTKVideoCodecType::VTKVC_VP9;
  bool Initialized = false;
  vtkRenderWindow* GraphicsContext = nullptr;
  // handler
  std::function<void(VTKVideoDecoderResultType)> OutputHandler = nullptr;
  int Width = 0;
  int Height = 0;

  ///@{
  /**
   * Concrete subclasses must handle initialization, allocation and freeing of decoder resources.
   * The subclass must also translate the implementation error code to one of
   * VTKVideoProcessingStatusType.
   */
  virtual bool InitializeInternal() = 0;
  virtual void ShutdownInternal() = 0;
  ///@

  ///@{
  /**
   * Concrete subclasses implement the process of decoding.
   */
  virtual VTKVideoDecoderResultType DecodeInternal(
    vtkSmartPointer<vtkCompressedVideoPacket> packet) = 0;
  virtual VTKVideoDecoderResultType SendEOS() = 0;
  ///@}

private:
  vtkVideoDecoder(const vtkVideoDecoder&) = delete;
  void operator=(const vtkVideoDecoder&) = delete;

  bool UpdateDecoderContext(int width, int height);
};

#endif // vtkVideoDecoder_h
