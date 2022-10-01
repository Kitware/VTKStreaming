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

#include <fstream>
#include <vector>

#define ALIGN_UP(s, a) ((s + a - 1) & ~(a - 1))

//------------------------------------------------------------------------------
vtkRawVideoFrame::vtkRawVideoFrame() = default;

//------------------------------------------------------------------------------
vtkRawVideoFrame::~vtkRawVideoFrame() = default;

//------------------------------------------------------------------------------
void vtkRawVideoFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "DisplayWidth: " << this->DisplayWidth << "\n";
  os << "DisplayHeight: " << this->DisplayHeight << "\n";
  os << "StorageWidth: " << this->StorageWidth << "\n";
  os << "StorageHeight: " << this->StorageHeight << "\n";
  os << "Strides: " << '\n';
  for (int i = 0; i < 3; ++i)
  {
    os << i << ':' << this->Strides[i] << '\n';
  }
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
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetWidth(int value) noexcept
{
  vtkLogScopeFunction(TRACE);
  this->DisplayWidth = value;
  this->StorageWidth = ALIGN_UP(value, 8);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkRawVideoFrame::GetWidth() const noexcept
{
  return this->DisplayWidth;
}

//------------------------------------------------------------------------------
int vtkRawVideoFrame::GetStorageWidth() const noexcept
{
  return this->StorageWidth;
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::SetHeight(int value) noexcept
{
  vtkLogScopeFunction(TRACE);
  this->DisplayHeight = value;
  this->StorageHeight = ALIGN_UP(value, 8);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkRawVideoFrame::GetHeight() const noexcept
{
  return this->DisplayHeight;
}

//------------------------------------------------------------------------------
int vtkRawVideoFrame::GetStorageHeight() const noexcept
{
  return this->StorageHeight;
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
unsigned int vtkRawVideoFrame::AlignUp(int value, int bytes) noexcept
{
  return ALIGN_UP(value, bytes);
}

//------------------------------------------------------------------------------
unsigned int vtkRawVideoFrame::GetWidthBytes(int width, VTKPixelFormatType pixelFormat) noexcept
{
  const auto alignedW = ALIGN_UP(width, 8);
  switch (pixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
    case VTKPixelFormatType::VTKPF_NV12:
      return alignedW;
    case VTKPixelFormatType::VTKPF_RGB24:
      return alignedW * 3;
    case VTKPixelFormatType::VTKPF_RGBA32:
      return alignedW * 4;
    default:
      return 0;
  }
}

//------------------------------------------------------------------------------
unsigned int vtkRawVideoFrame::GetNumberOfChromaPlanes(VTKPixelFormatType pixelFormat) noexcept
{
  switch (pixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
      return 2;
    case VTKPixelFormatType::VTKPF_NV12:
      return 1;
    case VTKPixelFormatType::VTKPF_RGB24:
    case VTKPixelFormatType::VTKPF_RGBA32:
    default:
      return 0;
  }
}

//------------------------------------------------------------------------------
unsigned int vtkRawVideoFrame::GetChromaHeight(int height, VTKPixelFormatType pixelFormat) noexcept
{
  switch (pixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
    case VTKPixelFormatType::VTKPF_NV12:
      return ALIGN_UP(height, 8) >> 1;
    case VTKPixelFormatType::VTKPF_RGB24:
    case VTKPixelFormatType::VTKPF_RGBA32:
    default:
      return 0;
  }
}

//------------------------------------------------------------------------------
unsigned int vtkRawVideoFrame::GetEstimatedSize(
  int width, int height, VTKPixelFormatType pixelFormat, int* strides /*=nullptr*/) noexcept
{
  unsigned int size = 0;
  const auto alignedW = ALIGN_UP(width, 8);
  const auto alignedH = ALIGN_UP(height, 8);
  if (strides == nullptr)
  {
    switch (pixelFormat)
    {
      case VTKPixelFormatType::VTKPF_IYUV:
      case VTKPixelFormatType::VTKPF_NV12:
        size = alignedW * (alignedH + (alignedH >> 1));
        break;
      case VTKPixelFormatType::VTKPF_RGB24:
        size = 3 * alignedW * alignedH;
        break;
      case VTKPixelFormatType::VTKPF_RGBA32:
        size = 4 * alignedW * alignedH;
        break;
    }
  }
  else
  {
    const auto chromaHeight = vtkRawVideoFrame::GetChromaHeight(height, pixelFormat);
    // clang-format off
     size = strides[0] * alignedH 
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
      chromaPitch = vtkRawVideoFrame::GetPitch(width, pixelFormat) >> 1;
      break;
    case VTKPixelFormatType::VTKPF_NV12:
      chromaPitch = vtkRawVideoFrame::GetPitch(width, pixelFormat) >> 1;
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
  const auto widthBytes = vtkRawVideoFrame::GetWidthBytes(this->DisplayWidth, this->PixelFormat);
  const auto chromaPitch = vtkRawVideoFrame::GetChromaPitch(this->DisplayWidth, this->PixelFormat);
  this->Strides[0] = widthBytes;
  this->Strides[1] = chromaPitch;
  this->Strides[2] = chromaPitch;
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

//------------------------------------------------------------------------------
void vtkRawVideoFrame::CopyPlanarData(unsigned char* from, int rowsize, int numrows, int plane)
{
  this->CopyPlanarDataInternal(from, rowsize, numrows, plane);
}

//------------------------------------------------------------------------------
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
  for (unsigned int i = 0; i < size; ++i)
  {
    data->SetValue(i, dataPtr[i]);
  }

  return data;
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::ShallowCopy(vtkRawVideoFrame* from) noexcept
{
  vtkLogScopeFunction(TRACE);
  this->DisplayWidth = from->DisplayWidth;
  this->DisplayHeight = from->DisplayHeight;
  this->StorageWidth = from->StorageWidth;
  this->StorageHeight = from->StorageHeight;
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
  this->DisplayWidth = from->DisplayWidth;
  this->DisplayHeight = from->DisplayHeight;
  this->StorageWidth = from->StorageWidth;
  this->StorageHeight = from->StorageHeight;
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
