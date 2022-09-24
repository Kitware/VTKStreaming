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

#include <algorithm>
#include <fstream>
#include <vector>

//------------------------------------------------------------------------------
vtkRawVideoFrame::vtkRawVideoFrame() = default;

//------------------------------------------------------------------------------
vtkRawVideoFrame::~vtkRawVideoFrame() = default;

//------------------------------------------------------------------------------
void vtkRawVideoFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Width: " << this->Width << "\n";
  os << "Height: " << this->Height << "\n";
  os << "PixelFormat: " << vtkPixelFormatTypeUtilities::ToString(this->PixelFormat) << '\n';
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

  os << "Strides: " << '\n';
  for (int i = 0; i < 3; ++i)
  {
    os << i << ':' << this->Strides[i] << '\n';
  }
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetWidth(int value) noexcept
{
  vtkLogScopeFunction(TRACE);
  this->Width = value;
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkRawVideoFrame::GetWidth() const noexcept
{
  return this->Width;
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetHeight(int value) noexcept
{
  vtkLogScopeFunction(TRACE);
  this->Height = value;
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkRawVideoFrame::GetHeight() const noexcept
{
  return this->Height;
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetPixelFormat(VTKPixelFormatType value) noexcept
{
  vtkLogScopeFunction(TRACE);
  this->PixelFormat = value;
  this->Modified();
}

//------------------------------------------------------------------------------
VTKPixelFormatType vtkRawVideoFrame::GetPixelFormat() const noexcept
{
  return this->PixelFormat;
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetSliceOrderType(vtkRawVideoFrame::SliceOrderType value) noexcept
{
  vtkLogScopeFunction(TRACE);
  this->SliceOrder = value;
  this->Modified();
}

//------------------------------------------------------------------------------
vtkRawVideoFrame::SliceOrderType vtkRawVideoFrame::GetSliceOrderType() const noexcept
{
  return this->SliceOrder;
}

//------------------------------------------------------------------------------
unsigned int vtkRawVideoFrame::GetWidthBytes(int width, VTKPixelFormatType pixelFormat) noexcept
{
  int widthBytes = 0;
  switch (pixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
    case VTKPixelFormatType::VTKPF_NV12:
      widthBytes = width;
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
      widthBytes = width * 3;
      break;
    case VTKPixelFormatType::VTKPF_RGBA32:
      widthBytes = width * 4;
      break;
  }
  return widthBytes;
}

//------------------------------------------------------------------------------
unsigned int vtkRawVideoFrame::GetNumberOfChromaPlanes(VTKPixelFormatType pixelFormat) noexcept
{
  unsigned int numCrPlanes = 0;
  switch (pixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
      numCrPlanes = 2;
      break;
    case VTKPixelFormatType::VTKPF_NV12:
      numCrPlanes = 1;
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
    case VTKPixelFormatType::VTKPF_RGBA32:
      break;
  }
  return numCrPlanes;
}

//------------------------------------------------------------------------------
unsigned int vtkRawVideoFrame::GetChromaHeight(int height, VTKPixelFormatType pixelFormat) noexcept
{
  unsigned int chromaHeight = 0;
  switch (pixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
    case VTKPixelFormatType::VTKPF_NV12:
      chromaHeight = (height + 1) >> 1;
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
    case VTKPixelFormatType::VTKPF_RGBA32:
      break;
  }
  return chromaHeight;
}

//------------------------------------------------------------------------------
unsigned int vtkRawVideoFrame::GetEstimatedSize(
  int width, int height, VTKPixelFormatType pixelFormat, int* strides /*=nullptr*/) noexcept
{
  unsigned int size = 0;
  if (strides == nullptr)
  {
    switch (pixelFormat)
    {
      case VTKPixelFormatType::VTKPF_IYUV:
      case VTKPixelFormatType::VTKPF_NV12:
        size = width * (height + ((height + 1) >> 1));
        break;
      case VTKPixelFormatType::VTKPF_RGB24:
        size = 3 * width * height;
        break;
      case VTKPixelFormatType::VTKPF_RGBA32:
        size = 4 * width * height;
        break;
    }
  }
  else
  {
    const auto chromaHeight = vtkRawVideoFrame::GetChromaHeight(height, pixelFormat);
    // clang-format off
     size = strides[0] * height 
          + strides[1] * chromaHeight 
          + strides[2] * chromaHeight;
    // clang-format on
  }
  return size;
}

//------------------------------------------------------------------------------
unsigned int vtkRawVideoFrame::GetPitch(int width, VTKPixelFormatType pixelFormat) noexcept
{
  return vtkRawVideoFrame::GetWidthBytes(width, pixelFormat);
}

//------------------------------------------------------------------------------
unsigned int vtkRawVideoFrame::GetChromaPitch(int width, VTKPixelFormatType pixelFormat) noexcept
{
  unsigned int chromaPitch = 0;
  switch (pixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
      chromaPitch = (vtkRawVideoFrame::GetPitch(width, pixelFormat) + 1) >> 1;
      break;
    case VTKPixelFormatType::VTKPF_NV12:
      chromaPitch = vtkRawVideoFrame::GetPitch(width, pixelFormat);
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
    case VTKPixelFormatType::VTKPF_RGBA32:
      chromaPitch = 0;
      break;
  }
  return chromaPitch;
}

//------------------------------------------------------------------------------
std::vector<unsigned int> vtkRawVideoFrame::GetChromaOffsets(
  int width, int height, VTKPixelFormatType pixelFormat) noexcept
{
  std::vector<unsigned int> offsets;
  const auto pitch = vtkRawVideoFrame::GetPitch(width, pixelFormat);
  const auto chromaPitch = vtkRawVideoFrame::GetChromaPitch(width, pixelFormat);
  const auto chromaHeight = vtkRawVideoFrame::GetChromaHeight(height, pixelFormat);
  switch (pixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
      offsets.emplace_back(pitch * height);
      offsets.emplace_back(offsets[0] + chromaPitch * chromaHeight);
      break;
    case VTKPixelFormatType::VTKPF_NV12:
      offsets.emplace_back(vtkRawVideoFrame::GetPitch(width, pixelFormat) * height);
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
    case VTKPixelFormatType::VTKPF_RGBA32:
      break;
  }
  return offsets;
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::ComputeDefaultStrides()
{
  const auto widthBytes = vtkRawVideoFrame::GetWidthBytes(this->Width, this->PixelFormat);
  this->Strides[0] = widthBytes;
  switch (this->PixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
    case VTKPixelFormatType::VTKPF_NV12:
      this->Strides[1] = widthBytes >> 1;
      this->Strides[2] = widthBytes >> 1;
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
    case VTKPixelFormatType::VTKPF_RGBA32:
      this->Strides[1] = 0;
      this->Strides[2] = 0;
      break;
  }
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetStrides(int* strides, int size)
{
  vtkLogScopeFunction(TRACE);
  for (int i = 0; i < 3 && i < size; ++i)
  {
    this->Strides[i] = strides[i];
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetStrides(int stride0, int stride1, int stride2)
{
  this->Strides[0] = stride0;
  this->Strides[1] = stride1;
  this->Strides[2] = stride2;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::CopyData(unsigned char* from, unsigned int size)
{
  vtkLogScopeFunction(TRACE);
  this->CopyDataInternal(from, size);
  this->Modified();
}

unsigned int vtkRawVideoFrame::GetData(unsigned char*& data)
{
  vtkLogScopeFunction(TRACE);
  return this->GetDataInternal(data);
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::CopyData(vtkUnsignedCharArray* from)
{
  vtkLogScopeFunction(TRACE);
  if (from == nullptr)
  {
    return;
  }
  this->CopyData(from->GetPointer(0), from->GetNumberOfValues());
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkUnsignedCharArray> vtkRawVideoFrame::GetData()
{
  vtkLogScopeFunction(TRACE);
  auto data = vtk::TakeSmartPointer(vtkUnsignedCharArray::New());

  unsigned char* dataPtr = nullptr;
  unsigned int size = this->GetData(dataPtr);
  data->SetNumberOfValues(size);
  std::copy(dataPtr, dataPtr + size, data->GetPointer(0));

  return data;
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::ShallowCopy(vtkRawVideoFrame* from) noexcept
{
  vtkLogScopeFunction(TRACE);
  this->Width = from->Width;
  this->Height = from->Height;
  this->PixelFormat = from->PixelFormat;
  this->SliceOrder = from->SliceOrder;
  for (int i = 0; i < 3; ++i)
  {
    this->Strides[i] = from->Strides[i];
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::DeepCopy(vtkRawVideoFrame* from)
{
  vtkLogScopeFunction(TRACE);
  this->Width = from->Width;
  this->Height = from->Height;
  this->PixelFormat = from->PixelFormat;
  this->SliceOrder = from->SliceOrder;
  for (int i = 0; i < 3; ++i)
  {
    this->Strides[i] = from->Strides[i];
  }
  this->AllocateDataStore();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::Save(const char* filename)
{
  vtkLogScopeFunction(TRACE);
  std::ofstream file(filename, std::ofstream::out | std::ofstream::binary);
  unsigned char* data = nullptr;
  const unsigned int size = this->GetData(data);
  file.write(reinterpret_cast<char*>(data), size);
}
