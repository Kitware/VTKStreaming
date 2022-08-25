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
 * @sa vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkAbstractVideoEncoder_h
#define vtkAbstractVideoEncoder_h

#include "vtkVideoCoreModule.h"

#include "vtkCodecTypes.h"
#include "vtkCommand.h"
#include "vtkObject.h"

#include <memory>

class vtkRawVideoFrame;
class vtkCodedVideoPacket;

class VTKVIDEOCORE_EXPORT vtkAbstractVideoEncoder : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractVideoEncoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * Public interface for the encoder. Concrete sub-classes are supposed to
   * implmement the respective *Internal() methods.
   * vtkAbstractVideoEncoder::Push(vtkRawVideoFrame* frame) is the entry point
   * to start encoding.
   *
   * Listen to `vtkCommand::ProgressEvent` to receive the compressed video packet.
   *
   * DevNote: Subclasses should invoke `vtkAbstractVideoEncoder::PacketHandler(vtkCodedVideoPacket*
   * pkt)` immediately after the implementation produces a coded video packet. The subclass must
   * wrap the implementation's packet object into a vtkCodedVideoPacket instance and then invoke
   * `PacketHandler`
   */
  bool Initialize();
  void Shutdown();
  void Flush();
  bool Push(vtkRawVideoFrame* frame);
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

  bool ForceIFrame = false;
  VTKCodecType Codec = VP9;
  int GroupOfPicturesSize = 10;
  int MaximumBFrames = 0;
  int TimeBaseStart = 1;
  int TimeBaseEnd = 30;
  unsigned int BitRate = 2000000; // 2Mbps

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
  virtual bool SetupEncoderFrame(const int& w, const int& h) = 0;
  virtual bool NeedsNewEncoderFrame(const int& w, const int& h) = 0;
  virtual void TearDownEncoderFrame() = 0;
  ///@}

  ///@{
  /**
   * Concrete subclasses implement the process of encoding and compressed packet retrieval.
   */
  virtual bool Encode() = 0;
  virtual bool PushInternal(vtkRawVideoFrame* frame) = 0;
  virtual void PacketHandler(vtkCodedVideoPacket* pkt);
  ///@}

private:
  vtkAbstractVideoEncoder(const vtkAbstractVideoEncoder&) = delete;
  void operator=(const vtkAbstractVideoEncoder&) = delete;
};

#endif // vtkAbstractVideoEncoder_h
