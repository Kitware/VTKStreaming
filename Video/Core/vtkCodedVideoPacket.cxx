/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCodedVideoPacket.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCodedVideoPacket.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCodedVideoPacket);

//------------------------------------------------------------------------------
vtkCodedVideoPacket::vtkCodedVideoPacket() = default;

//------------------------------------------------------------------------------
vtkCodedVideoPacket::~vtkCodedVideoPacket() = default;

//------------------------------------------------------------------------------
void vtkCodedVideoPacket::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Size: " << this->Buffer->GetNumberOfValues() << "\n";
  os << "IsKeyFrame: " << this->IsKeyFrame << "\n";
  os << "PresentationTS: " << this->PresentationTS << "\n";
  os << "Width: " << this->Width << "\n";
  os << "Height: " << this->Height << "\n";
}

//------------------------------------------------------------------------------
void vtkCodedVideoPacket::SetSize(int size)
{
  this->Buffer->SetNumberOfValues(size);
}

//------------------------------------------------------------------------------
int vtkCodedVideoPacket::GetSize() const
{
  return this->Buffer->GetNumberOfValues();
}

//------------------------------------------------------------------------------
void vtkCodedVideoPacket::SetArray(unsigned char* buffer, int size)
{
  this->Buffer->SetArray(buffer, size, 1);
}

//------------------------------------------------------------------------------
void vtkCodedVideoPacket::SetArray(vtkUnsignedCharArray* buffer)
{
  if (buffer != nullptr)
  {
    this->SetArray(buffer->GetPointer(0), buffer->GetNumberOfValues());
  }
}

//------------------------------------------------------------------------------
void vtkCodedVideoPacket::CopyData(unsigned char* buffer, int size)
{
  this->Buffer->SetNumberOfValues(size);
  std::copy(buffer, buffer + size, this->Buffer->GetPointer(0));
}

//------------------------------------------------------------------------------
void vtkCodedVideoPacket::CopyData(vtkUnsignedCharArray* buffer)
{
  if (buffer != nullptr)
  {
    this->SetArray(buffer->GetPointer(0), buffer->GetNumberOfValues());
  }
}

//------------------------------------------------------------------------------
int vtkCodedVideoPacket::GetData(unsigned char*& buffer) const
{
  buffer = this->Buffer->GetPointer(0);
  return this->Buffer->GetNumberOfValues();
}
