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
 * @sa vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkAbstractVideoDecoder_h
#define vtkAbstractVideoDecoder_h

#include "vtkVideoCoreModule.h"

#include "vtkCodecTypes.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPixelFormats.h"
#include "vtkRawVideoFrame.h"

#include <memory>

class vtkCodedVideoPacket;

class VTKVIDEOCORE_EXPORT vtkAbstractVideoDecoder : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractVideoDecoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetEnumMacro(CodecType, VTKCodecType);
  vtkGetEnumMacro(CodecType, VTKCodecType);

  vtkSetEnumMacro(OutputPixelFormat, VTKPixelFormat);
  vtkGetEnumMacro(OutputPixelFormat, VTKPixelFormat);

  ///@{
  /**
   * Public interface for the decoder. Concrete sub-classes are supposed to
   * implmement the respective *Internal() methods.
   * vtkAbstractVideoDecoder::Push(vtkCodedVideoPacket* packet) is the entry point
   * to start decoding.
   *
   * Listen to `vtkCommand::ProgressEvent` to receive the decompressed video frame.
   */
  bool Initialize();
  void Shutdown();
  void Flush();
  bool Push(vtkCodedVideoPacket* packet);
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

  ///@{
  /**
   * Concrete subclasses must handle initialization, allocation and freeing of decoder resources.
   */
  virtual bool InitializeInternal() = 0;
  virtual void ShutdownInternal() = 0;
  virtual void FlushInternal() = 0;
  ///@}

  ///@{
  /**
   * Concrete subclasses implement the process of decoding.
   */
  virtual bool Decode() = 0;
  virtual bool PushInternal(vtkCodedVideoPacket* packet) = 0;
  ///@}

private:
  vtkAbstractVideoDecoder(const vtkAbstractVideoDecoder&) = delete;
  void operator=(const vtkAbstractVideoDecoder&) = delete;
};

#endif // vtkAbstractVideoDecoder_h
