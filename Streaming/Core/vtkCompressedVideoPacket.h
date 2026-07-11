// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
/**
 * @class   vtkCompressedVideoPacket
 * @brief   class that encapsulates chunk of encoded video produced by a video encoder
 *
 * vtkCompressedVideoPacket associates a chunk of encoded video with
 * key parameters such as width, height, presentation time stamp.
 * It also has a flag to indicate whether a chunk of encoded video corresponds
 * to a key frame or not.
 *
 * @sa vtkAbstractVideoencoder, vtkVideoDecoder
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
   * Specifies the mime type, codec type of the binary blob in this packet.
   * Ex: "video/webm; codecs="vp09.00.10.08"", "image/bmp"
   * Ex: CodecLongName = "vp09.00.10.08"
   */
  void SetMimeType(const char* value);
  std::string GetMimeType() const;
  void SetCodecLongName(const char* value);
  std::string GetCodecLongName() const;
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
   * Set/Get width at which the packet must be presented for display.
   */
  vtkGetMacro(DisplayWidth, int);
  vtkSetMacro(DisplayWidth, int);
  ///@}

  ///@{
  /**
   * Set/Get width at which the packet is coded. (may differ from display due to alignment)
   */
  vtkGetMacro(CodedWidth, int);
  vtkSetMacro(CodedWidth, int);
  ///@}

  ///@{
  /**
   * Set/Get height at which the packet must be presented for display.
   */
  vtkGetMacro(DisplayHeight, int);
  vtkSetMacro(DisplayHeight, int);
  ///@}

  ///@{
  /**
   * Set/Get height at which the packet is coded. (may differ from display due to alignment)
   */
  vtkGetMacro(CodedHeight, int);
  vtkSetMacro(CodedHeight, int);
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
   * Return the contents of a chunk of encoded video.
   */
  int GetData(unsigned char*& buffer) const;
  vtkUnsignedCharArray* GetData() const { return this->Buffer; };
  ///@}

  ///@{
  /**
   * Copy the array that represents a chunk of encoded video.
   * Allocates sufficient number of bytes if necessary.
   */
  virtual void CopyData(unsigned char* buffer, int size);
  void CopyData(vtkUnsignedCharArray* buffer);
  ///@}

  ///@{
  /**
   * Makes a copy of all members except the data.
   * Use the more expressive AllocateForCopy/CopyData/GetData functions to manage the
   * underlying buffer.
   */
  void CopyMetadata(vtkCompressedVideoPacket* other);
  void AllocateForCopy(vtkCompressedVideoPacket* other);
  ///@}

protected:
  vtkCompressedVideoPacket();
  ~vtkCompressedVideoPacket() override;

  bool IsKeyFrame = false;
  int DisplayWidth = 0;
  int DisplayHeight = 0;
  int CodedWidth = 0;
  int CodedHeight = 0;
  long long PresentationTS = 0;
  std::string MimeType;
  std::string CodecLongName;
  vtkNew<vtkUnsignedCharArray> Buffer;

private:
  vtkCompressedVideoPacket(const vtkCompressedVideoPacket&) = delete;
  void operator=(const vtkCompressedVideoPacket&) = delete;
};

#endif // vtkCompressedVideoPacket_h
