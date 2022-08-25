/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRawVideoFrame.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRawVideoFrame.h"
#include "vtkLogger.h"
#include "vtkPixelFormats.h"
#include <algorithm>

vtkStandardNewMacro(vtkRawVideoFrame);

//------------------------------------------------------------------------------
vtkRawVideoFrame::vtkRawVideoFrame() = default;

//------------------------------------------------------------------------------
vtkRawVideoFrame::~vtkRawVideoFrame() = default;

//------------------------------------------------------------------------------
void vtkRawVideoFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Size: " << this->Buffer->GetNumberOfValues() << "\n";
  os << "IsKeyFrame: " << this->IsKeyFrame << "\n";
  os << "PresentationTS: " << this->PresentationTS << "\n";
  os << "Width: " << this->Width << "\n";
  os << "Height: " << this->Height << "\n";

  os << "PixelFormat: ";
  switch (this->PixelFormat)
  {
    case VTKPixelFormat::NV12:
      os << "NV12\n";
    case VTKPixelFormat::RGB24:
      os << "RGB24\n";
    case VTKPixelFormat::RGBA32:
      os << "RGBA32\n";
    case VTKPixelFormat::YUV420P:
      os << "YUV420P\n";
    default:
      break;
  }
  os << "SliceOrder: ";
  switch (this->SliceOrder)
  {
    case vtkRawVideoFrame::SliceOrderType::TopDown:
      os << "TopDown\n";
    case vtkRawVideoFrame::SliceOrderType::BottomUp:
    default:
      os << "BottomUp\n";
      break;
  }
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetSize(int size)
{
  this->Buffer->SetNumberOfValues(size);
}

//------------------------------------------------------------------------------
int vtkRawVideoFrame::GetSize()
{
  return this->Buffer->GetNumberOfValues();
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetArray(unsigned char* buffer, int size)
{
  this->Buffer->SetArray(buffer, size, 1);
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetArray(vtkUnsignedCharArray* buffer)
{
  if (buffer != nullptr)
  {
    this->SetArray(buffer->GetPointer(0), buffer->GetNumberOfValues());
  }
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::CopyData(unsigned char* buffer, int size)
{
  this->Buffer->SetNumberOfValues(size);
  std::copy(buffer, buffer + size, this->Buffer->GetPointer(0));
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::CopyData(vtkUnsignedCharArray* buffer)
{
  if (buffer != nullptr)
  {
    this->SetArray(buffer->GetPointer(0), buffer->GetNumberOfValues());
  }
}

//------------------------------------------------------------------------------
int vtkRawVideoFrame::GetData(unsigned char*& buffer) const
{
  buffer = this->Buffer->GetPointer(0);
  return this->Buffer->GetNumberOfValues();
}
