/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRawVideoFrame.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRawVideoFrame
 * @brief   class that represents a raw video frame.
 *
 * vtkRawVideoFrame associates an un-compressed video frame with
 * key parameters such as width, height, presentation time stamp,
 * pixel format and slice order.
 * It also has a flag to indicate whether the uncompressed frame corresponds
 * to a key frame or not.
 *
 * The slice order is the memory representation of an image. It can be either top-down
 * or bottom-up. In a top-down ordering, the first block in memory corresponds to the pixel
 * in top-left corner, whereas, in a bottom-up ordering, the first block in memory corresponds
 * to a pixel in the bottom-left corner.
 *
 * Video encoders and decoders work with a top-down memory ordering. OpenGL and DirectX, BMP
 * are known to be represented in bottom-up memory order.
 *
 * Calling code can let an instance of this class manage memory
 * for the compressed packet by using the `CopyData` overload instead of `SetArray`.
 *
 * The `SetArray` overload may be helpful when working with a GPU mapped buffer.
 *
 * @sa vtkCodedVideoPacket, vtkAbstractVideoEncoder, vtkAbstractVideoDecoder, vtkPixelFormats
 */

#ifndef vtkRawVideoFrame_h
#define vtkRawVideoFrame_h

#include "vtkVideoCoreModule.h"

#include "vtkObject.h"
#include "vtkPixelFormats.h"
#include "vtkUnsignedCharArray.h"

class VTKVIDEOCORE_EXPORT vtkRawVideoFrame : public vtkObject
{
public:
  vtkTypeMacro(vtkRawVideoFrame, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkRawVideoFrame* New();

  enum SliceOrderType
  {
    TopDown,  // <- YUV, planar video frames
    BottomUp, // <- OpenGL pixels, BMP
  };

  ///@{
  /**
   * Set to true if the frame is a key frame.
   */
  vtkGetMacro(IsKeyFrame, bool);
  vtkSetMacro(IsKeyFrame, bool);
  ///@}

  ///@{
  /**
   * Set/Get width of the frame.
   */
  vtkGetMacro(Width, int);
  vtkSetMacro(Width, int);
  ///@}

  ///@{
  /**
   * Set/Get height of the frame.
   */
  vtkGetMacro(Height, int);
  vtkSetMacro(Height, int);
  ///@}

  ///@{
  /**
   * Set/Get the presentation timestamp of the frame.
   * This is an integer that spans the encoder timebase [1, fps]
   */
  vtkGetMacro(PresentationTS, long long);
  vtkSetMacro(PresentationTS, long long);
  ///@}

  ///@{
  /**
   * Set/Get the format of the pixels. Ex: RGB24, RGBA32, YUV420P, NV12
   */
  vtkGetEnumMacro(PixelFormat, VTKPixelFormat);
  vtkSetEnumMacro(PixelFormat, VTKPixelFormat);
  ///@}

  ///@{
  /**
   * Set/Get the slice order of the image.
   */
  vtkGetEnumMacro(SliceOrder, SliceOrderType);
  vtkSetEnumMacro(SliceOrder, SliceOrderType);
  ///@}

  ///@{
  /**
   * Set/Get the size of an internal buffer.
   * Use the SetSize variant when you wish to prepare the frame from an external data pointer.
   */
  virtual void SetSize(int size);
  virtual int GetSize();
  ///@}

  ///@{
  /**
   * Set the array that represents image pixels.
   * This method does NOT save the array contents.
   * Remember to consume the buffer from GetData(buf) before
   * the owner frees this buffer.
   */
  virtual void SetArray(unsigned char* buffer, int size);
  void SetArray(vtkUnsignedCharArray* buffer);
  virtual int GetData(unsigned char*& buffer) const;
  ///@}

  ///@{
  /**
   * Copy the array that represents image pixels.
   * This method saves the array contents by copying the elements.
   */
  virtual void CopyData(unsigned char* buffer, int size);
  void CopyData(vtkUnsignedCharArray* buffer);
  ///@}

protected:
  vtkRawVideoFrame();
  ~vtkRawVideoFrame() override;

  bool IsKeyFrame = false;
  int Width = 0;
  int Height = 0;
  long long PresentationTS = 0;
  VTKPixelFormat PixelFormat;
  SliceOrderType SliceOrder;
  vtkNew<vtkUnsignedCharArray> Buffer;

private:
  vtkRawVideoFrame(const vtkRawVideoFrame&) = delete;
  void operator=(const vtkRawVideoFrame&) = delete;
};

#endif // vtkRawVideoFrame
