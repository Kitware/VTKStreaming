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
#include "vtkPixelFormatTypes.h"

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
  vtkLogScopeF(TRACE, "%s, w=%d", __func__, value);
  this->DisplayWidth = value;
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
  vtkLogScopeF(TRACE, "%s, h=%d", __func__, value);
  this->DisplayHeight = value;
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
  vtkLogScopeF(TRACE, "%s, pix_fmt=%s", __func__, vtkPixelFormatTypeUtilities::ToString(value));
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
  vtkLogScopeF(TRACE, "%s, slice_order=%d", __func__, value);
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
  switch (pixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
    case VTKPixelFormatType::VTKPF_NV12:
      return AlignUp(width, 8);
    case VTKPixelFormatType::VTKPF_RGB24:
      return width * 3;
    case VTKPixelFormatType::VTKPF_RGBA32:
      return width * 4;
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
      return AlignUp(height, 8) >> 1;
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
  const auto alignedW = AlignUp(width, 8);
  const auto alignedH = AlignUp(height, 8);
  if (strides == nullptr)
  {
    switch (pixelFormat)
    {
      case VTKPixelFormatType::VTKPF_IYUV:
      case VTKPixelFormatType::VTKPF_NV12:
        size = alignedW * (alignedH + (alignedH >> 1));
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
    const bool packed = (pixelFormat == VTKPixelFormatType::VTKPF_RGB24) ||
      (pixelFormat == VTKPixelFormatType::VTKPF_RGBA32);
    // clang-format off
     size = strides[0] * (packed ? height : alignedH)
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
  switch (this->PixelFormat)
  {
    case VTKPixelFormatType::VTKPF_IYUV:
    case VTKPixelFormatType::VTKPF_NV12:
      this->StorageWidth = AlignUp(this->DisplayWidth, 8);
      this->StorageHeight = AlignUp(this->DisplayHeight, 8);
      break;
    default:
      this->StorageWidth = this->DisplayWidth;
      this->StorageHeight = this->DisplayHeight;
      break;
  }
  this->Strides[0] = widthBytes;
  this->Strides[1] = chromaPitch;
  this->Strides[2] = chromaPitch;
}

//------------------------------------------------------------------------------
int vtkRawVideoFrame::GetPlanePointerIdx(int planeIdx)
{
  int idx = 0;
  using PFT = VTKPixelFormatType;
  const auto chromaHeight = this->GetChromaHeight(this->DisplayHeight, this->PixelFormat);
  if (this->PixelFormat == PFT::VTKPF_RGB24 || this->PixelFormat == PFT::VTKPF_RGBA32)
  {
    return 0;
  }
  if (planeIdx == 0)
  {
    return 0;
  }
  idx += this->Strides[0] * this->StorageHeight;
  if (planeIdx == 1)
  {
    return idx;
  }
  // planeIdx == 2
  if (this->PixelFormat == PFT::VTKPF_IYUV)
  {
    idx += this->Strides[0] * (chromaHeight >> 1);
  }
  return idx;
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::CopyData(unsigned char* from, int rowsize, int numrows)
{
  vtkLogScopeFunction(TRACE);
  this->CopyDataInternal(from, rowsize, numrows);
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
void vtkRawVideoFrame::CopyData(vtkUnsignedCharArray* from, int rowsize, int numrows)
{
  vtkLogScopeFunction(TRACE);
  if (from == nullptr)
  {
    return;
  }
  this->CopyData(from->GetPointer(0), rowsize, numrows);
}

//------------------------------------------------------------------------------
void vtkRawVideoFrame::CopyPlanarData(
  vtkUnsignedCharArray* from, int rowsize, int numrows, int plane)
{
  vtkLogScopeFunction(TRACE);
  if (from == nullptr)
  {
    return;
  }
  this->CopyPlanarData(from->GetPointer(0), rowsize, numrows, plane);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkUnsignedCharArray> vtkRawVideoFrame::GetData()
{
  vtkLogScopeFunction(TRACE);
  auto data = vtk::TakeSmartPointer(vtkUnsignedCharArray::New());

  unsigned char* dptr = nullptr;
  unsigned int size = this->GetData(dptr);
  switch (this->PixelFormat)
  {
    case VTKPixelFormatType::VTKPF_RGBA32:
      data->SetNumberOfComponents(4);
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
      data->SetNumberOfComponents(3);
      break;
    case VTKPixelFormatType::VTKPF_IYUV:
    case VTKPixelFormatType::VTKPF_NV12:
    default:
      data->SetNumberOfComponents(1);
      break;
  }
  data->SetArray(dptr, size, 0);
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
  auto array = this->GetData();
  file.write(reinterpret_cast<char*>(array->GetPointer(0)), array->GetNumberOfValues());
}
