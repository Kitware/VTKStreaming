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
  for (int i = 0; i < VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES; ++i)
  {
    os << "Plane-" << i << " Size: " << this->Planes[i]->GetNumberOfValues() << "\n";
  }
  os << "IsKeyFrame: " << this->IsKeyFrame << "\n";
  os << "PresentationTS: " << this->PresentationTS << "\n";
  os << "Width: " << this->Width << "\n";
  os << "Height: " << this->Height << "\n";

  os << "PixelFormat: ";
  switch (this->PixelFormat)
  {
    case VTKPixelFormat::NV12:
      os << "NV12\n";
      break;
    case VTKPixelFormat::RGB24:
      os << "RGB24\n";
      break;
    case VTKPixelFormat::RGBA32:
      os << "RGBA32\n";
      break;
    case VTKPixelFormat::YUV420P:
      os << "YUV420P\n";
      break;
    default:
      break;
  }
  os << "SliceOrder: ";
  switch (this->SliceOrder)
  {
    case vtkRawVideoFrame::SliceOrderType::TopDown:
      os << "TopDown\n";
      break;
    case vtkRawVideoFrame::SliceOrderType::BottomUp:
    default:
      os << "BottomUp\n";
      break;
  }
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::ShallowCopy(vtkRawVideoFrame* other)
{
  for (int i = 0; i < VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES; ++i)
  {
    this->Strides[i] = other->Strides[i];
  }
  this->IsKeyFrame = other->IsKeyFrame;
  this->PresentationTS = other->PresentationTS;
  this->Width = other->Width;
  this->Height = other->Height;
  this->PixelFormat = other->PixelFormat;
  this->SliceOrder = other->SliceOrder;
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::AllocateCopy(vtkRawVideoFrame* other)
{
  for (int i = 0; i < VTK_RAW_VIDEO_FRAME_MAX_NUM_PLANES; ++i)
  {
    this->Planes[i]->SetNumberOfValues(other->Planes[i]->GetNumberOfValues());
  }
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetSize(int size, int plane /*=0*/)
{
  this->Planes[plane]->SetNumberOfValues(size);
}

//------------------------------------------------------------------------------
int vtkRawVideoFrame::GetSize(int plane /*=0*/) const
{
  return this->Planes[plane]->GetNumberOfValues();
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetArray(unsigned char* buffer, int size, int plane /*=0*/)
{
  this->Planes[plane]->SetArray(buffer, size, 1);
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetArray(vtkUnsignedCharArray* buffer, int plane /*=0*/)
{
  if (buffer != nullptr)
  {
    this->SetArray(buffer->GetPointer(0), buffer->GetNumberOfValues(), plane);
  }
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::CopyData(unsigned char* buffer, int size, int plane /*=0*/)
{
  std::copy(buffer, buffer + size, this->Planes[plane]->GetPointer(0));
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::CopyData(vtkUnsignedCharArray* buffer, int plane /*=0*/)
{
  if (buffer != nullptr)
  {
    this->CopyData(buffer->GetPointer(0), buffer->GetNumberOfValues(), plane);
  }
}

//------------------------------------------------------------------------------
int vtkRawVideoFrame::GetData(unsigned char*& buffer, int plane /*=0*/) const
{
  buffer = this->Planes[plane]->GetPointer(0);
  return this->Planes[plane]->GetNumberOfValues();
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetStrides(int* strides, int size)
{
  for (int i = 0; i < 8 && i < size; ++i)
  {
    this->Strides[i] = strides[i];
  }
  this->Modified();
  this->StridesMTime = this->GetMTime();
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::ComputeStrides(int byteAlignment /*=1*/)
{
  if (this->GetMTime() <= this->StridesMTime)
  {
    return;
  }

  auto getPaddedSize = [&byteAlignment](const int& size)
  { return size + (byteAlignment - (size % byteAlignment)) % byteAlignment; };

  switch (this->PixelFormat)
  {
    case RGBA32:
      this->Strides[0] = getPaddedSize(4 * this->Width);
      break;
    case RGB24:
      this->Strides[0] = getPaddedSize(3 * this->Width);
      break;
    case YUV420P:
      this->Strides[0] = getPaddedSize(this->Width);
      this->Strides[1] = getPaddedSize(this->Width >> 1);
      this->Strides[2] = getPaddedSize(this->Width >> 1);
      break;
    case NV12:
      this->Strides[0] = getPaddedSize(this->Width);
      this->Strides[1] = getPaddedSize(this->Strides[0] >> 1);
      break;
    default:
      vtkLog(ERROR, << "Unsupported pixel format");
  }
  this->Modified();
  this->StridesMTime = this->GetMTime();
}
