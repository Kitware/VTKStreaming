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
 * @brief   abstract class for a raw video frame that leaves out storage implementation
 *          for the subclasses.
 *
 * vtkRawVideoFrame associates an un-compressed video frame with
 * key parameters such as width, height, luminance-chroma pitches, offsets,
 * pixel format and slice order.
 *
 * @sa vtkCPUVideoFrame, vtkOpenGLVideoFrame
 */

#ifndef vtkRawVideoFrame_h
#define vtkRawVideoFrame_h

#include "vtkObject.h"

#include "vtkPixelFormatTypes.h"    // for pixel type enum
#include "vtkSmartPointer.h"        // for return value
#include "vtkStreamingCoreModule.h" // for export macro
#include "vtkUnsignedCharArray.h"   // for return value

class vtkRenderWindow;

class VTKSTREAMINGCORE_EXPORT vtkRawVideoFrame : public vtkObject
{
public:
  vtkTypeMacro(vtkRawVideoFrame, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum SliceOrderType
  {
    TopDown,  // <- Video codec expected storage format.
    BottomUp, // <- OpenGL pixels, BMP
  };

  ///@{
  /**
   * Set to true if this is a key frame. For information puposes.
   */
  vtkGetMacro(IsKeyFrame, bool);
  vtkSetMacro(IsKeyFrame, bool);
  ///@}

  ///@{
  /**
   * Set/Get width of the frame.
   */
  void SetWidth(int) noexcept;
  int GetWidth() const noexcept;
  ///@}

  ///@{
  /**
   * Set/Get height of the frame.
   */
  void SetHeight(int) noexcept;
  int GetHeight() const noexcept;
  ///@}

  ///@{
  /**
   * Set/Get the format of the pixels. Ex: RGB24, RGBA32, IYUV, NV12
   */
  void SetPixelFormat(VTKPixelFormatType) noexcept;
  VTKPixelFormatType GetPixelFormat() const noexcept;
  ///@}

  ///@{
  /**
   * Set/Get the slice order of the image.
   *
   * The slice order is the memory representation of an image. It can be either top-down
   * or bottom-up. In a top-down ordering, the first block in memory corresponds to the pixel
   * in top-left corner, whereas, in a bottom-up ordering, the first block in memory corresponds
   * to a pixel in the bottom-left corner.
   *
   * Video encoders and decoders work with a top-down memory ordering, whereas
   * OpenGL framebuffers and textures are represented in bottom-up memory order.
   */
  void SetSliceOrderType(SliceOrderType) noexcept;
  SliceOrderType GetSliceOrderType() const noexcept;
  ///@}

  ///@{
  /**
   * Set/Get the strides and use them to interpret the underlying data.
   * Refer https://docs.microsoft.com/en-us/windows/win32/medfound/image-stride for an
   * excellent description of planar image strides.
   */
  void ComputeDefaultStrides();
  void SetStrides(int* strides, int size);
  void SetStrides(int stride0, int stride1, int stride2);
  int* GetStrides() VTK_SIZEHINT(3) { return this->Strides; }
  ///@}

  ///@{
  /**
   * Concrete subclasses can implement pixel read/write operation
   * from/to the given render window.
   * This is an opportunity for subclasses with GPU-storage to
   * avoid GPU->CPU transfer during the read/write operation.
   */
  virtual void Capture(vtkRenderWindow* window) = 0;
  virtual void Render(vtkRenderWindow* window) = 0;
  ///@}

  ///@{
  /**
   * Methods that help encoding with a single GPU->GPU copy.
   *
   * When an encoder finds an attached render window, it shall use ::Capture(attachedWindow)
   * to get the pixels in a single GPU->GPU tranfser.
   */
  void Attach(vtkRenderWindow* window) noexcept;
  void Detach() noexcept;
  vtkRenderWindow* GetAttachedRenderWindow() const noexcept { return this->AttachedWindow; };
  ///@}

  /**
   * Convenient method that writes out underlying memory to a file.
   */
  void Save(const char* filename);

  /**
   * Copy data from another memory address into this frame.
   */
  void CopyData(unsigned char* from, unsigned int size);

  /**
   * Get a pointer to underlying data. returns the size in bytes.
   */
  unsigned int GetData(unsigned char*& data) const;

  ///@{
  /**
   * Copy/Get the pixel data from/to a vtkUnsignedCharArray.
   */
  void CopyData(vtkUnsignedCharArray* from);
  vtkSmartPointer<vtkUnsignedCharArray> GetData() const;
  ///@}

  /**
   * Copy another frame's data into our frame.
   * If none of the pixel-format and dimensions are equal, it will fail.
   */
  void CopyFrameData(vtkRawVideoFrame* from);

  /**
   * Get the actual size instead of estimated size. Often, the data can be
   * larger than our estimated size due to presence of extra padded bytes.
   */
  virtual unsigned int GetActualSize() const = 0;

  /**
   * Allocates internal storage based on the estimated size of this frame.
   */
  virtual void AllocateDataStore() = 0;

  /**
   * Derived instances can return device pointer here.
   * ex: glTexture/cuda/d3d handles.
   * It can be anything you want it to be to help your encoder/decoder implementation.
   */
  virtual void* GetResourceHandle() noexcept = 0;

  /**
   * Copy members except the underlying buffer contents.
   */
  virtual void CopyMetadata(vtkRawVideoFrame* from) noexcept;

  ///@{
  /**
   * Luminance/chrominance utilities.
   * Get pitch of the frame.
   * Get chroma pitch of the frame.
   * Get chroma plane offsets of the frame.
   */
  static unsigned int GetWidthBytes(int width, VTKPixelFormatType pixelFormat) noexcept;
  static unsigned int GetNumberOfChromaPlanes(VTKPixelFormatType pixelFormat) noexcept;
  static unsigned int GetChromaHeight(int height, VTKPixelFormatType pixelFormat) noexcept;
  static unsigned int GetEstimatedSize(
    int width, int height, VTKPixelFormatType pixelFormat, int* strides = nullptr) noexcept;
  static unsigned int GetPitch(int width, VTKPixelFormatType pixelFormat) noexcept;
  static unsigned int GetChromaPitch(int width, VTKPixelFormatType pixelFormat) noexcept;
  static std::vector<unsigned int> GetChromaOffsets(
    int width, int height, VTKPixelFormatType pixelFormat) noexcept;
  ///@}

protected:
  vtkRawVideoFrame();
  ~vtkRawVideoFrame() override;

  int Width = 0;
  int Height = 0;
  int Strides[3];
  bool IsKeyFrame = false;
  VTKPixelFormatType PixelFormat = VTKPixelFormatType::VTKPF_NV12;
  SliceOrderType SliceOrder = SliceOrderType::TopDown;
  vtkRenderWindow* AttachedWindow = nullptr;

  virtual void CopyDataInternal(unsigned char* from, unsigned int size) = 0;
  virtual unsigned int GetDataInternal(unsigned char*& data) const = 0;
  virtual void CopyFrameDataInternal(vtkRawVideoFrame* from) = 0;

private:
  vtkRawVideoFrame(const vtkRawVideoFrame&) = delete;
  void operator=(const vtkRawVideoFrame&) = delete;
};

#endif // vtkRawVideoFrame
