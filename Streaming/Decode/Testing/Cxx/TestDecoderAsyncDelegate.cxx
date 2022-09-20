/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDecoderDelegateUsage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test demonstrates the arrival of packets when a video decoder uses
// asynchronous delegate. The mock video decoder simulates a decoder lag
// by sleeping for a given time interval. You can increase the mock lag inteval
// and notice that arrival of frames is interspersed with Push calls.

#include "vtkCallbackCommand.h"
#include "vtkCompressedVideoPacket.h"
#include "vtkLogger.h"
#include "vtkMockVideoDecoder.h"

#include <chrono>
#include <thread>

#define MAX_NUM_PACKETS 100

static int readyCount = 0;
void callback(vtkObject*, unsigned long, void*, void*)
{
  vtkLogF(INFO, "=> ready %d", readyCount++);
}

int TestDecoderAsyncDelegate(int argc, char* argv[])
{
  vtkNew<vtkMockVideoDecoder> decoder;
  bool success = true;
  std::vector<vtkNew<vtkCompressedVideoPacket>> packets(MAX_NUM_PACKETS);

  decoder->SetMockDecodeTimeMilliseconds(MAX_NUM_PACKETS / 10);
  decoder->SetMockLargePacketPeriod(MAX_NUM_PACKETS / 20);
  decoder->SetMockLargePacketIntervalRatio(MAX_NUM_PACKETS / 2);

  decoder->UseAsynchronousDelegateOn();

  vtkNew<vtkCallbackCommand> cmd;
  cmd->SetCallback(callback);
  decoder->AddObserver(vtkCommand::ProgressEvent, cmd);

  int pushCount = 0;
  for (const auto& packet : packets)
  {
    packet->SetSize(4096 * 2160);
    vtkLogF(INFO, "Push %d", pushCount++);
    decoder->Push(packet);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  decoder->Flush();
  decoder->Shutdown();
  success = readyCount == pushCount;
  if (!success)
  {
    vtkLogF(ERROR, "Failed readyCount != pushCount | %d != %d", readyCount, pushCount);
    return 1;
  }

  return 0;
}
