/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMKVWriterImplementation.h

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkMKVWriterImplementation_h
#define vtkMKVWriterImplementation_h

#include "vtkStreamingWEBMModule.h"
#include "vtkUnsignedCharArray.h" // for ivar

#include "vtkstreaming_libwebm.h"

#include <cstdio>

// Implement the abstract IMkvWriter interface.
class VTKSTREAMINGWEBM_EXPORT vtkMKVWriterImplementation : public mkvmuxer::IMkvWriter
{
public:
  vtkMKVWriterImplementation();
  explicit vtkMKVWriterImplementation(bool writeToMem);
  ~vtkMKVWriterImplementation() override;

  // IMkvWriter interface
  mkvmuxer::int64 Position() const override;
  mkvmuxer::int32 Position(mkvmuxer::int64 position) override;
  bool Seekable() const override;
  mkvmuxer::int32 Write(const void* buffer, mkvmuxer::uint32 length) override;
  void ElementStartNotify(mkvmuxer::uint64 element_id, mkvmuxer::int64 position) override;

  bool Open(const char* filename);
  void Close();
  vtkUnsignedCharArray* GetDynamicBuffer();

private:
  bool WriteToMemory = false;
  std::FILE* OutputFile;
  vtkNew<vtkUnsignedCharArray> DynamicBuffer;
  LIBWEBM_DISALLOW_COPY_AND_ASSIGN(vtkMKVWriterImplementation);
};

#endif // vtkMKVWriterImplementation_h
// VTK-HeaderTest-Exclude: vtkMKVWriterImplementation.h
