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
// This test exercises the API of vtkAbstractDecoder when it is instructed
// to bypass the processing delegate.
// The aim is to verify that Push/GetResult do not use a delegate.

#include "vtkCompressedVideoPacket.h"
#include "vtkIndent.h"
#include "vtkLogger.h"
#include "vtkMockVideoDecoder.h"
#include "vtkSmartPointer.h"

int TestDecoderDelegateUsage(int argc, char* argv[])
{
  // 1. UseAsynchronousDelegateOn
  {
    vtkLog(TRACE, << "1. UseAsynchronousDelegateOn");
    vtkNew<vtkMockVideoDecoder> decoder;
    bool success = true;

    decoder->UseAsynchronousDelegateOn();
    success &= decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 1. UseAsynchronousDelegateOn");
      return 1;
    }
  }

  // 2. UseAsynchronousDelegateOff
  {
    vtkLog(TRACE, << "2. UseAsynchronousDelegateOff");
    vtkNew<vtkMockVideoDecoder> decoder;
    bool success = true;

    decoder->UseAsynchronousDelegateOff();
    success &= !decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 2. UseAsynchronousDelegateOff");
      return 1;
    }
  }

  // 3. UseAsynchronousDelegateOn,Push,UseAsynchronousDelegateOff
  {
    vtkLog(TRACE, << "3. UseAsynchronousDelegateOn,Push,UseAsynchronousDelegateOff");
    vtkNew<vtkMockVideoDecoder> decoder;
    vtkNew<vtkCompressedVideoPacket> packet;
    packet->SetWidth(4);
    packet->SetHeight(4);

    bool success = true;
    decoder->UseAsynchronousDelegateOn();
    success &= decoder->HasDelegate();

    decoder->Push(packet);
    success &= decoder->HasDelegate();

    decoder->UseAsynchronousDelegateOff();
    success &= !decoder->HasDelegate();

    decoder->Push(packet);
    success &= !decoder->HasDelegate();

    if (!success)
    {
      vtkLog(ERROR, << "Failed 3. UseAsynchronousDelegateOn,Push,UseAsynchronousDelegateOff");
      return 1;
    }
  }

  // 4. UseAsynchronousDelegateOff,Push,UseAsynchronousDelegateOn
  {
    vtkLog(TRACE, << "4. UseAsynchronousDelegateOff,Push,UseAsynchronousDelegateOn");
    vtkNew<vtkMockVideoDecoder> decoder;
    vtkNew<vtkCompressedVideoPacket> packet;
    packet->SetWidth(4);
    packet->SetHeight(4);

    bool success = true;
    decoder->UseAsynchronousDelegateOff();
    success &= !decoder->HasDelegate();

    decoder->Push(packet);
    success &= !decoder->HasDelegate();

    decoder->UseAsynchronousDelegateOn();
    success &= decoder->HasDelegate();

    decoder->Push(packet);
    success &= decoder->HasDelegate();

    if (!success)
    {
      vtkLog(ERROR, << "Failed 4. UseAsynchronousDelegateOff,Push,UseAsynchronousDelegateOn");
      return 1;
    }
  }

  return 0;
}
