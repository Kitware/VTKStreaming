/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCodedVideoPacket.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCodedVideoPacket
 * @brief   class that encapsulates compressed video packets produced by a video encoder
 *
 * vtkCodedVideoPacket associates a compressed video packet bitstream with
 * key parameters such as width, height, presentation time stamp.
 * It also has a flag to indicate whether the compressed packet corresponds
 * to a key frame or not.
 *
 * Calling code can let an instance of this class manage memory
 * for the compressed packet by using the `CopyData` overload instead of SetArray.
 *
 * @sa vtkRawVideoFrame, vtkAbstractVideoencoder, vtkAbstractVideoDecoder
 */

#ifndef vtkCodedVideoPacket_h
#define vtkCodedVideoPacket_h

#include "vtkVideoCoreModule.h"

#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkUnsignedCharArray.h"

class VTKVIDEOCORE_EXPORT vtkCodedVideoPacket : public vtkObject
{
public:
  vtkTypeMacro(vtkCodedVideoPacket, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkCodedVideoPacket* New();

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
   * Set/Get the size of an internal buffer.
   * Use the SetSize variant when you wish to prepare the frame from an external data pointer.
   */
  void SetSize(int size);
  int GetSize() const;
  ///@}

  ///@{
  /**
   * Set the array that represents the compressed video packet.
   * This method does NOT save the array contents.
   * Remember to consume the buffer from GetData(buf) before
   * the owner frees this buffer.
   */
  virtual void SetArray(unsigned char* buffer, int size);
  void SetArray(vtkUnsignedCharArray* buffer);
  int GetData(unsigned char*& buffer) const;
  vtkUnsignedCharArray* GetData() const { return this->Buffer; };
  ///@}

  ///@{
  /**
   * Copy the array that represents the compressed video packet.
   * This method saves the array contents by copying the elements.
   */
  virtual void CopyData(unsigned char* buffer, int size);
  void CopyData(vtkUnsignedCharArray* buffer);
  ///@}

protected:
  vtkCodedVideoPacket();
  ~vtkCodedVideoPacket() override;

  bool IsKeyFrame = false;
  int Width = 0;
  int Height = 0;
  long long PresentationTS = 0;
  vtkNew<vtkUnsignedCharArray> Buffer;

private:
  vtkCodedVideoPacket(const vtkCodedVideoPacket&) = delete;
  void operator=(const vtkCodedVideoPacket&) = delete;
};

#endif // vtkCodedVideoPacket_h
