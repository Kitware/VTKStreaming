/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCPUVideoFrame.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCPUVideoFrame.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPixelFormatTypes.h"

#include <algorithm>

vtkStandardNewMacro(vtkCPUVideoFrame);

//------------------------------------------------------------------------------
vtkCPUVideoFrame::vtkCPUVideoFrame() = default;

//------------------------------------------------------------------------------
vtkCPUVideoFrame::~vtkCPUVideoFrame() = default;

//------------------------------------------------------------------------------
void vtkCPUVideoFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Size: " << this->GetActualSize();
  os << "Buffer: \n";
  this->Buffer->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkCPUVideoFrame::Render(vtkRenderWindow* window)
{
  vtkLog(ERROR,
    "vtkCPUVideoFrame cannot render itself into vtkRenderWindow. Use vtkOpenGLVideoFrame instead.");
}

//------------------------------------------------------------------------------
void vtkCPUVideoFrame::Capture(vtkRenderWindow* window)
{
  vtkLog(ERROR,
    "vtkCPUVideoFrame cannot load itself from a vtkRenderWindow. Use vtkOpenGLVideoFrame instead.");
}

//------------------------------------------------------------------------------
void vtkCPUVideoFrame::CopyDataInternal(unsigned char* from, unsigned int size)
{
  vtkLogScopeF(TRACE, "%s, size=%d, actual_size=%d", __func__, size, this->GetActualSize());
  const int allocSize = this->GetActualSize();
  if (size > allocSize)
  {
    this->Buffer->Allocate(size);
    std::copy(from, from + size, this->Buffer->GetBuffer());
  }
  else if (size < allocSize)
  {
    vtkLog(TRACE, << "Padding reason: Given data size " << size << " smaller than frame size "
                  << allocSize << " - " << this->Strides[0] << "x" << this->Height
                  << " for pixel format "
                  << vtkPixelFormatTypeUtilities::ToString(this->PixelFormat));
    std::copy(from, from + size, this->Buffer->GetBuffer());
    // pad with zeros.
    std::fill(this->Buffer->GetBuffer() + size, this->Buffer->GetBuffer() + allocSize, 0);
  }
  else
  {
    std::copy(from, from + size, this->Buffer->GetBuffer());
  }
}

//------------------------------------------------------------------------------
unsigned int vtkCPUVideoFrame::GetDataInternal(unsigned char*& data)
{
  vtkLogScopeFunction(TRACE);
  data = this->Buffer->GetBuffer();
  return this->Buffer->GetSize();
}

//------------------------------------------------------------------------------
unsigned int vtkCPUVideoFrame::GetActualSize() const
{
  return this->Buffer->GetSize();
}

//------------------------------------------------------------------------------
void vtkCPUVideoFrame::AllocateDataStore()
{
  vtkLogScopeFunction(TRACE);
  const int allocSize =
    vtkRawVideoFrame::GetEstimatedSize(this->Width, this->Height, this->PixelFormat, this->Strides);
  this->Buffer->Allocate(allocSize);
}

//------------------------------------------------------------------------------
void vtkCPUVideoFrame::ShallowCopy(vtkRawVideoFrame* from) noexcept
{
  vtkLogScopeFunction(TRACE);
  this->Superclass::ShallowCopy(from);

  unsigned char* srcData = nullptr;
  auto size = from->GetData(srcData);

  this->Buffer->SetBuffer(srcData, size);
  this->Buffer->SetFreeFunction(true, nullptr);
}

//------------------------------------------------------------------------------
void vtkCPUVideoFrame::DeepCopy(vtkRawVideoFrame* from)
{
  vtkLogScopeFunction(TRACE);
  this->Superclass::DeepCopy(from);

  unsigned char* srcData = nullptr;
  const unsigned int srcSize = from->GetData(srcData);
  if (this->SliceOrder == from->GetSliceOrderType())
  {
    this->CopyData(srcData, srcSize);
    return;
  }
  else
  {
    vtkLog(TRACE, "Slice orders do not match. Will invert picture along Y dimension.");
    std::vector<unsigned char> flippedContents(srcSize);
    const auto widthBytes = vtkRawVideoFrame::GetWidthBytes(this->Strides[0], this->PixelFormat);
    const auto chromaHeight = vtkRawVideoFrame::GetChromaHeight(this->Height, this->PixelFormat);
    for (unsigned int row = 0; row < this->Height; ++row)
    {
      const auto flippedRow = this->Height - 1 - row;
      for (unsigned int i = 0; i < widthBytes; ++i)
      {
        flippedContents[row * widthBytes + i] = srcData[flippedRow * widthBytes + i];
      }
    }
    for (unsigned int row = this->Height; row < chromaHeight; ++row)
    {
      const auto flippedRow = this->Height - 1 - row;
      for (unsigned int i = 0; i < widthBytes; ++i)
      {
        flippedContents[row * widthBytes + i] = srcData[flippedRow * widthBytes + i];
      }
    }
    this->CopyData(flippedContents.data(), srcSize);
  }
}
