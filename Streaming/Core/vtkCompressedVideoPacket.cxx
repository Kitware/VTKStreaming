/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompressedVideoPacket.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCompressedVideoPacket.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCompressedVideoPacket);

//------------------------------------------------------------------------------
vtkCompressedVideoPacket::vtkCompressedVideoPacket() = default;

//------------------------------------------------------------------------------
vtkCompressedVideoPacket::~vtkCompressedVideoPacket() = default;

//------------------------------------------------------------------------------
void vtkCompressedVideoPacket::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "MimeType: " << this->MimeType << "\n";
  os << "CodecLongName: " << this->CodecLongName << "\n";
  os << "Size: " << this->Buffer->GetNumberOfValues() << "\n";
  os << "IsKeyFrame: " << this->IsKeyFrame << "\n";
  os << "PresentationTS: " << this->PresentationTS << "\n";
  os << "DisplayWidth: " << this->DisplayWidth << "\n";
  os << "DisplayHeight: " << this->DisplayHeight << "\n";
  os << "CodedWidth: " << this->CodedWidth << "\n";
  os << "CodedHeight: " << this->CodedHeight << "\n";
}

//------------------------------------------------------------------------------
void vtkCompressedVideoPacket::SetMimeType(const char* value)
{
  this->MimeType = value != nullptr ? value : "";
  this->Modified();
}

//------------------------------------------------------------------------------
std::string vtkCompressedVideoPacket::GetMimeType() const
{
  return this->MimeType;
}

//------------------------------------------------------------------------------
void vtkCompressedVideoPacket::SetCodecLongName(const char* value)
{
  this->CodecLongName = value != nullptr ? value : "";
  this->Modified();
}

//------------------------------------------------------------------------------
std::string vtkCompressedVideoPacket::GetCodecLongName() const
{
  return this->CodecLongName;
}

//------------------------------------------------------------------------------
void vtkCompressedVideoPacket::SetSize(int size)
{
  this->Buffer->SetNumberOfValues(size);
}

//------------------------------------------------------------------------------
int vtkCompressedVideoPacket::GetSize() const
{
  return this->Buffer->GetNumberOfValues();
}

//------------------------------------------------------------------------------
void vtkCompressedVideoPacket::CopyData(unsigned char* buffer, int size)
{
  if (this->Buffer->GetNumberOfValues() != size)
  {
    // clear out previous contents.
    this->Buffer->SetNumberOfValues(size);
  }
  std::copy(buffer, buffer + size, this->Buffer->GetPointer(0));
}

//------------------------------------------------------------------------------
void vtkCompressedVideoPacket::CopyData(vtkUnsignedCharArray* buffer)
{
  if (buffer != nullptr)
  {
    this->CopyData(buffer->GetPointer(0), buffer->GetNumberOfValues());
  }
}

//------------------------------------------------------------------------------
int vtkCompressedVideoPacket::GetData(unsigned char*& buffer) const
{
  buffer = this->Buffer->GetPointer(0);
  return this->Buffer->GetNumberOfValues();
}

//------------------------------------------------------------------------------
void vtkCompressedVideoPacket::CopyMetadata(vtkCompressedVideoPacket* other)
{
  if (other == nullptr)
  {
    return;
  }
  this->MimeType = other->MimeType;
  this->CodecLongName = other->CodecLongName;
  this->IsKeyFrame = other->IsKeyFrame;
  this->PresentationTS = other->PresentationTS;
  this->DisplayWidth = other->DisplayWidth;
  this->DisplayHeight = other->DisplayHeight;
  this->CodedWidth = other->CodedWidth;
  this->CodedHeight = other->CodedHeight;
}

//------------------------------------------------------------------------------
void vtkCompressedVideoPacket::AllocateForCopy(vtkCompressedVideoPacket* other)
{
  if (other == nullptr)
  {
    return;
  }
  this->Buffer->SetNumberOfValues(other->Buffer->GetNumberOfValues());
}
