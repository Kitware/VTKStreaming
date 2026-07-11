// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
// This test exercises the memory access API for vtkCompressedVideoPacket

#include "vtkCompressedVideoPacket.h"
#include "vtkLogger.h"
#include "vtkStreamingTestUtility.h"

#include <vector>

int TestCodedVideoPacketMemory(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
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

  // 4. AllocateForCopy/CopyData(unsigned char*, int)
  {
    vtkLog(TRACE, << "4. AllocateForCopy/CopyData(unsigned char*, int)");
    success = true;
    vtkNew<vtkCompressedVideoPacket> srcPacket, dstPacket;
    auto buf = std::vector<unsigned char>(6);
    for (int i = 0; i < 6; ++i)
    {
      buf[i] = i;
    }
    srcPacket->SetSize(6);
    srcPacket->CopyData(buf.data(), 6);

    dstPacket->AllocateForCopy(srcPacket);
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
      vtkLog(ERROR, << "Failed 4. AllocateForCopy/CopyData(unsigned char*, int)");
      return 1;
    }
  }

  // 5. AllocateForCopy/CopyData(vtkUnsignedCharArray*)
  {
    vtkLog(TRACE, << "5. AllocateForCopy/CopyData(vtkUnsignedCharArray*)");
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

    dstPacket->AllocateForCopy(srcPacket);
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
      vtkLog(ERROR, << "Failed 5. AllocateForCopy/CopyData(vtkUnsignedCharArray*)");
      return 1;
    }
  }

  return 0;
}
