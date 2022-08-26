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
#include "vtkType.h"
#include "vtkUnsignedCharArray.h"
#include <vector>

#define VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES 8

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
   * plane value: 0-VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES
   */
  virtual void SetSize(int size, int plane = 0);
  virtual int GetSize(int plane = 0) const;
  ///@}

  ///@{
  /**
   * Set the array that represents image pixels.
   * This method does NOT save the array contents.
   * Remember to consume the buffer from GetData(buf) before
   * the owner frees this buffer.
   * plane value: 0-VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES
   * Use the plane argument to specify a higher (>0) plane for planar image formats like YUV, NV12.
   * In packed image formats like RGBA32 and RGB24, all the data is in the Planes[0] buffer.
   * In planar image formats like YUV and friends, the Y data is in Planes[0], U -> Planes[1], V ->
   * Planes[2]. This class can represent upto 8 planes.
   */
  virtual void SetArray(unsigned char* buffer, int size, int plane = 0);
  void SetArray(vtkUnsignedCharArray* buffer, int plane = 0);
  virtual int GetData(unsigned char*& buffer, int plane = 0) const;
  ///@}

  ///@{
  /**
   * Copy the array that represents image pixels.
   * This method saves the array contents by copying the elements.
   * plane value: 0-VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES
   */
  virtual void CopyData(unsigned char* buffer, int size, int plane = 0);
  void CopyData(vtkUnsignedCharArray* buffer, int plane = 0);
  ///@}

  ///@{
  /**
   * Computes strides with specified byte alignment.
   * You can also set the strides externally and use them to interpret the planes.
   * In case, you explicitly called SetStrides, make sure that you don't modify the object
   * before the next GetStrides() call.
   * Refer https://docs.microsoft.com/en-us/windows/win32/medfound/image-stride for an
   * excellent description on this topic.
   */
  void ComputeStrides(int byteAignment = 1);
  void SetStrides(int* strides, int size);
  void SetStrides(std::vector<int> strides) 
  {
    this->SetStrides(strides.data(), static_cast<int>(strides.size()));
  }
  int* GetStrides() VTK_SIZEHINT(VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES)
  {
    this->ComputeStrides();
    return this->Strides;
  }
  ///@}

protected:
  vtkRawVideoFrame();
  ~vtkRawVideoFrame() override;

  bool IsKeyFrame = false;
  int Width = 0;
  int Height = 0;
  int Strides[VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES];
  vtkMTimeType StridesMTime = 0;
  long long PresentationTS = 0;
  VTKPixelFormat PixelFormat;
  SliceOrderType SliceOrder;
  vtkNew<vtkUnsignedCharArray> Planes[VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES];

private:
  vtkRawVideoFrame(const vtkRawVideoFrame&) = delete;
  void operator=(const vtkRawVideoFrame&) = delete;
};

#endif // vtkRawVideoFrame
