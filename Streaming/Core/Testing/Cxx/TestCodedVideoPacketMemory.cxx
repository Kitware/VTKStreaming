/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCodedVideoPacketMemory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises the memory access API for vtkCompressedVideoPacket

#include "vtkCompressedVideoPacket.h"
#include "vtkLogger.h"

#include <vector>

int TestCodedVideoPacketMemory(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool success = true;
  // 1. Set/GetSize
  {
    vtkLog(TRACE, << "1. Set/GetSize");
    success = true;
    vtkNew<vtkCompressedVideoPacket> packet;
    packet->SetSize(4);
    success &= (packet->GetData()->GetNumberOfValues() == 4);
    success &= (packet->GetSize() == 4);
    if (!success)
    {
      vtkLog(ERROR, << "Failed 1. Set/GetSize");
      return 1;
    }
  }

  // 2. SetSize/CopyData(unsigned char*, int)
  {
    vtkLog(TRACE, << "2. SetSize/CopyData(unsigned char*, int)");
    success = true;
    vtkNew<vtkCompressedVideoPacket> packet;
    auto buf = std::vector<unsigned char>(6);
    for (int i = 0; i < 6; ++i)
    {
      buf[i] = i;
    }
    packet->SetSize(6);
    packet->CopyData(buf.data(), 6);
    success &= (packet->GetSize() == 6);
    success &= (packet->GetData()->GetNumberOfValues() == 6);

    for (int i = 0; i < 6; ++i)
    {
      success &= (packet->GetData()->GetValue(i) == buf[i]);
    }
    if (!success)
    {
      vtkLog(ERROR, << "Failed 2. SetSize/CopyData(unsigned char*, int)");
      return 1;
    }
  }

  // 3. SetSize/CopyData(vtkUnsignedCharArray*)
  {
    vtkLog(TRACE, << "3. SetSize/CopyData(vtkUnsignedCharArray*)");
    success = true;
    vtkNew<vtkCompressedVideoPacket> packet;
    vtkNew<vtkUnsignedCharArray> buf;
    buf->SetNumberOfValues(6);
    for (int i = 0; i < 6; ++i)
    {
      buf->SetValue(i, i);
    }
    packet->SetSize(6);
    packet->CopyData(buf);
    success &= (packet->GetSize() == 6);
    success &= (packet->GetData()->GetNumberOfValues() == 6);

    for (int i = 0; i < 6; ++i)
    {
      success &= (packet->GetData()->GetValue(i) == buf->GetValue(i));
    }
    if (!success)
    {
      vtkLog(ERROR, << "Failed 3. SetSize/CopyData(vtkUnsignedCharArray*)");
      return 1;
    }
  }

  // 4. AllocateCopy/CopyData(unsigned char*, int)
  {
    vtkLog(TRACE, << "4. AllocateCopy/CopyData(unsigned char*, int)");
    success = true;
    vtkNew<vtkCompressedVideoPacket> srcPacket, dstPacket;
    auto buf = std::vector<unsigned char>(6);
    for (int i = 0; i < 6; ++i)
    {
      buf[i] = i;
    }
    srcPacket->SetSize(6);
    srcPacket->CopyData(buf.data(), 6);

    dstPacket->AllocateCopy(srcPacket);
    success &= (dstPacket->GetSize() == 6);
    success &= (dstPacket->GetData()->GetNumberOfValues() == 6);

    dstPacket->CopyData(buf.data(), 6);
    for (int i = 0; i < 6; ++i)
    {
      success &= ((dstPacket->GetData()->GetValue(i) == buf[i]) &&
        (buf[i] == srcPacket->GetData()->GetValue(i)));
    }
    if (!success)
    {
      vtkLog(ERROR, << "Failed 4. AllocateCopy/CopyData(unsigned char*, int)");
      return 1;
    }
  }

  // 5. AllocateCopy/CopyData(vtkUnsignedCharArray*)
  {
    vtkLog(TRACE, << "5. AllocateCopy/CopyData(vtkUnsignedCharArray*)");
    success = true;
    vtkNew<vtkCompressedVideoPacket> srcPacket, dstPacket;
    vtkNew<vtkUnsignedCharArray> buf;
    buf->SetNumberOfValues(6);
    for (int i = 0; i < 6; ++i)
    {
      buf->SetValue(i, i);
    }
    srcPacket->SetSize(6);
    srcPacket->CopyData(buf);

    dstPacket->AllocateCopy(srcPacket);
    success &= (dstPacket->GetSize() == 6);
    success &= (dstPacket->GetData()->GetNumberOfValues() == 6);

    dstPacket->CopyData(buf);
    for (int i = 0; i < 6; ++i)
    {
      success &= ((dstPacket->GetData()->GetValue(i) == buf->GetValue(i)) &&
        (buf->GetValue(i) == srcPacket->GetData()->GetValue(i)));
    }
    if (!success)
    {
      vtkLog(ERROR, << "Failed 5. AllocateCopy/CopyData(vtkUnsignedCharArray*)");
      return 1;
    }
  }

  return 0;
}
