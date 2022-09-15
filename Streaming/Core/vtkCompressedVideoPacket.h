/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompressedVideoPacket.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompressedVideoPacket
 * @brief   class that encapsulates compressed video packets produced by a video encoder
 *
 * vtkCompressedVideoPacket associates a compressed video packet bitstream with
 * key parameters such as width, height, presentation time stamp.
 * It also has a flag to indicate whether the compressed packet corresponds
 * to a key frame or not.
 *
 * Calling code can let an instance of this class manage memory
 * for the compressed packet by using the `CopyData` overload instead of SetArray.
 *
 * @sa vtkCPUVideoFrame, vtkAbstractVideoencoder, vtkVideoDecoder
 */

#ifndef vtkCompressedVideoPacket_h
#define vtkCompressedVideoPacket_h

#include "vtkObject.h"

#include "vtkNew.h"                 // for ivar
#include "vtkStreamingCoreModule.h" // for export macro
#include "vtkUnsignedCharArray.h"   // for ivar

#include <string> // for ivar

class VTKSTREAMINGCORE_EXPORT vtkCompressedVideoPacket : public vtkObject
{
public:
  vtkTypeMacro(vtkCompressedVideoPacket, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkCompressedVideoPacket* New();

  ///@{
  /**
   * Specifies the mime type of the binary blob in this packet.
   * Ex: "video/webm; codecs="vp09.00.10.08"", "image/bmp"
   */
  void SetMimeType(const char* value);
  std::string GetMimeType() const;
  ///@}

  ///@{
  /**
   * Set to true if the compressed frame is a key frame.
   */
  vtkGetMacro(IsKeyFrame, bool);
  vtkSetMacro(IsKeyFrame, bool);
  ///@}

  ///@{
  /**
   * Set/Get width of the compressed frame.
   */
  vtkGetMacro(Width, int);
  vtkSetMacro(Width, int);
  ///@}

  ///@{
  /**
   * Set/Get height of the compressed frame.
   */
  vtkGetMacro(Height, int);
  vtkSetMacro(Height, int);
  ///@}

  ///@{
  /**
   * Set/Get the presentation timestamp of the compressed frame.
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
   * Return the contents of the compressed video packet.
   */
  int GetData(unsigned char*& buffer) const;
  vtkUnsignedCharArray* GetData() const { return this->Buffer; };
  ///@}

  ///@{
  /**
   * Copy the array that represents the compressed video packet.
   *
   * Assumes that underlying buffer is allocated to atleast 'size' bytes through
   * either vtkCompressedVideoPacket::SetSize or vtkCompressedVideoPacket::AllocateCopy
   */
  virtual void CopyData(unsigned char* buffer, int size);
  void CopyData(vtkUnsignedCharArray* buffer);
  ///@}

  ///@{
  /**
   * Makes a copy of all members except the data.
   * Use the more expressive AllocateCopy/SetArray/CopyData/GetData functions to manage the
   * underlying buffer.
   */
  void ShallowCopy(vtkCompressedVideoPacket* other);
  void AllocateCopy(vtkCompressedVideoPacket* other);
  ///@}

protected:
  vtkCompressedVideoPacket();
  ~vtkCompressedVideoPacket() override;

  bool IsKeyFrame = false;
  int Width = 0;
  int Height = 0;
  long long PresentationTS = 0;
  std::string MimeType;
  vtkNew<vtkUnsignedCharArray> Buffer;

private:
  vtkCompressedVideoPacket(const vtkCompressedVideoPacket&) = delete;
  void operator=(const vtkCompressedVideoPacket&) = delete;
};

#endif // vtkCompressedVideoPacket_h
