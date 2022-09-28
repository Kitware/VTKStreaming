/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMKVWriterImplementation.cxx

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMKVWriterImplementation.h"
#include "vtkLogger.h"

#include <exception>

#ifdef _MSC_VER
#include <share.h> // for _SH_DENYWR
#endif

//------------------------------------------------------------------------------
vtkMKVWriterImplementation::vtkMKVWriterImplementation() = default;

//------------------------------------------------------------------------------
vtkMKVWriterImplementation::vtkMKVWriterImplementation(bool writeToMem)
  : WriteToMemory(writeToMem)
{
}

//------------------------------------------------------------------------------
vtkMKVWriterImplementation::~vtkMKVWriterImplementation()
{
  this->Close();
}

//------------------------------------------------------------------------------
bool vtkMKVWriterImplementation::Open(const char* filename)
{
  vtkLogScopeFunction(TRACE);
#ifdef _MSC_VER
  this->OutputFile = _fsopen(filename, "wb", _SH_DENYWR);
#else
  this->OutputFile = fopen(filename, "wb");
#endif
  if (this->OutputFile == nullptr)
  {
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkMKVWriterImplementation::Close()
{
  vtkLogScopeFunction(TRACE);
  if (this->WriteToMemory)
  {
    return;
  }
  if (this->OutputFile != nullptr)
  {
    fclose(this->OutputFile);
  }
  this->OutputFile = nullptr;
}

//------------------------------------------------------------------------------
mkvmuxer::int64 vtkMKVWriterImplementation::Position() const
{
  vtkLogScopeFunction(TRACE);
  if (this->WriteToMemory)
  {
    return this->DynamicBuffer->GetNumberOfValues();
  }
  if (this->OutputFile == nullptr)
  {
    return 0;
  }
#ifdef _MSV_VER
  return _ftelli64(this->OutputFile);
#else
  return ftell(this->OutputFile);
#endif
}

//------------------------------------------------------------------------------
mkvmuxer::int32 vtkMKVWriterImplementation::Position(mkvmuxer::int64 position)
{
  vtkLogScopeFunction(TRACE);
  if (this->WriteToMemory)
  {
    vtkLog(ERROR, << "weird");
  }
  if (this->OutputFile == nullptr)
  {
    return -1;
  }

#ifdef _MSC_VER
  return _fseeki64(this->OutputFile, position, SEEK_SET);
#elif defined(_WIN32)
  return fseeko64(this->OutputFile, static_cast<off_t>(position), SEEK_SET);
#else
  return fseeko(this->OutputFile, static_cast<off_t>(position), SEEK_SET);
#endif
}

//------------------------------------------------------------------------------
bool vtkMKVWriterImplementation::Seekable() const
{
  vtkLogScopeFunction(TRACE);
  return false;
}

//------------------------------------------------------------------------------
mkvmuxer::int32 vtkMKVWriterImplementation::Write(const void* buffer, mkvmuxer::uint32 length)
{
  vtkLogScopeFunction(TRACE);
  if (length == 0)
  {
    return 0;
  }

  if (buffer == nullptr)
  {
    return -1;
  }

  if (this->OutputFile != nullptr && !this->WriteToMemory)
  {
    vtkLog(TRACE, << "Writing " << length << " bytes");
    const size_t bytes_written = fwrite(buffer, 1, length, this->OutputFile);
    vtkLog(TRACE, << "Wrote " << bytes_written << " bytes");
    return (bytes_written == length) ? 0 : -1;
  }
  else if (this->WriteToMemory)
  {
    vtkLog(TRACE, << "Writing " << length << " bytes");
    try
    {
      for (mkvmuxer::uint32 i = 0; i < length; ++i)
      {
        this->DynamicBuffer->InsertNextValue(static_cast<const unsigned char*>(buffer)[i]);
      }
      vtkLog(TRACE, << "Wrote " << length << " bytes");
      return 0;
    }
    catch (std::exception& e)
    {
      vtkLog(ERROR, << e.what());
      return -1;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
void vtkMKVWriterImplementation::ElementStartNotify(
  mkvmuxer::uint64 element_id, mkvmuxer::int64 position)
{
  vtkLogScopeFunction(TRACE);
  (void)element_id;
  (void)position;
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkMKVWriterImplementation::GetDynamicBuffer()
{
  vtkLogScopeFunction(TRACE);
  return this->DynamicBuffer;
}
