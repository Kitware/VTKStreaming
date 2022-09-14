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
#include <cstdlib>

int TestDecoderDelegateUsage(int argc, char* argv[])
{
  // 1. BypassDelegateOff,UseSynchronousDelegate
  {
    vtkLog(TRACE, << "1. BypassDelegateOff,UseSynchronousDelegate");
    vtkNew<vtkMockVideoDecoder> decoder;
    bool success = true;

    decoder->BypassDelegateOff();
    decoder->UseSynchronousDelegate();
    success &= decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 1. BypassDelegateOff,UseSynchronousDelegate");
      return 1;
    }
  }

  // 2. BypassDelegateOn,UseSynchronousDelegate
  {
    vtkLog(TRACE, << "2. BypassDelegateOn,UseSynchronousDelegate");
    vtkNew<vtkMockVideoDecoder> decoder;
    bool success = true;

    decoder->BypassDelegateOn();
    decoder->UseSynchronousDelegate();
    success &= !decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 2. BypassDelegateOn,UseSynchronousDelegate");
      return 1;
    }
  }

  // 3. UseSynchronousDelegate,BypassDelegateOff
  {
    vtkLog(TRACE, << "3. UseSynchronousDelegate,BypassDelegateOff");
    vtkNew<vtkMockVideoDecoder> decoder;
    bool success = true;

    decoder->UseSynchronousDelegate();
    decoder->BypassDelegateOff();
    success &= decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 3. UseSynchronousDelegate,BypassDelegateOff");
      return 1;
    }
  }

  // 4. UseSynchronousDelegate,BypassDelegateOn
  {
    vtkLog(TRACE, << "4. UseSynchronousDelegate,BypassDelegateOff");
    vtkNew<vtkMockVideoDecoder> decoder;
    bool success = true;

    decoder->UseSynchronousDelegate();
    decoder->BypassDelegateOn();
    success &= decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 4. UseSynchronousDelegate,BypassDelegateOff");
      return 1;
    }
  }

  // 5. UseSynchronousDelegate,BypassDelegateOff,Push
  {
    vtkLog(TRACE, << "5. UseSynchronousDelegate,BypassDelegateOff,Push");
    vtkNew<vtkMockVideoDecoder> decoder;
    vtkNew<vtkCompressedVideoPacket> packet;
    bool success = true;

    packet->SetSize(10);

    decoder->UseSynchronousDelegate();
    decoder->BypassDelegateOff();
    decoder->Push(packet);
    decoder->Flush();
    success &= decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 5. UseSynchronousDelegate,BypassDelegateOff,Push");
      return 1;
    }
  }

  // 6. UseSynchronousDelegate,BypassDelegateOn,Push
  {
    vtkLog(TRACE, << "6. UseSynchronousDelegate,BypassDelegateOn,Push");
    vtkNew<vtkMockVideoDecoder> decoder;
    vtkNew<vtkCompressedVideoPacket> packet;
    bool success = true;

    packet->SetSize(10);

    decoder->UseSynchronousDelegate();
    decoder->BypassDelegateOn();
    decoder->Push(packet);
    decoder->Flush();
    success &= !decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 6. UseSynchronousDelegate,BypassDelegateOn,Push");
      return 1;
    }
  }

  // 7. BypassDelegateOff,UseAsynchronousDelegate
  {
    vtkLog(TRACE, << "7. BypassDelegateOff,UseAsynchronousDelegate");
    vtkNew<vtkMockVideoDecoder> decoder;
    bool success = true;

    decoder->BypassDelegateOff();
    decoder->UseAsynchronousDelegate();
    success &= decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 7. BypassDelegateOff,UseAsynchronousDelegate");
      return 1;
    }
  }

  // 8. BypassDelegateOn,UseAsynchronousDelegate
  {
    vtkLog(TRACE, << "8. BypassDelegateOn,UseAsynchronousDelegate");
    vtkNew<vtkMockVideoDecoder> decoder;
    bool success = true;

    decoder->BypassDelegateOn();
    decoder->UseAsynchronousDelegate();
    success &= !decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 8. BypassDelegateOn,UseAsynchronousDelegate");
      return 1;
    }
  }

  // 9. UseAsynchronousDelegate,BypassDelegateOff
  {
    vtkLog(TRACE, << "9. UseAsynchronousDelegate,BypassDelegateOff");
    vtkNew<vtkMockVideoDecoder> decoder;
    bool success = true;

    decoder->UseAsynchronousDelegate();
    decoder->BypassDelegateOff();
    success &= decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 9. UseAsynchronousDelegate,BypassDelegateOff");
      return 1;
    }
  }

  // 10. UseAsynchronousDelegate,BypassDelegateOn
  {
    vtkLog(TRACE, << "10. UseAsynchronousDelegate,BypassDelegateOn");
    vtkNew<vtkMockVideoDecoder> decoder;
    bool success = true;

    decoder->UseAsynchronousDelegate();
    decoder->BypassDelegateOn();
    success &= decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 10. UseAsynchronousDelegate,BypassDelegateOn");
      return 1;
    }
  }

  // 11. UseAsynchronousDelegate,BypassDelegateOff,Push
  {
    vtkLog(TRACE, << "11. UseAsynchronousDelegate,BypassDelegateOff,Push");
    vtkNew<vtkMockVideoDecoder> decoder;
    vtkNew<vtkCompressedVideoPacket> packet;
    bool success = true;

    packet->SetSize(10);

    decoder->UseAsynchronousDelegate();
    decoder->BypassDelegateOff();
    decoder->Push(packet);
    decoder->Flush();
    success &= decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 11. UseAsynchronousDelegate,BypassDelegateOff,Push");
      return 1;
    }
  }

  // 12. UseAsynchronousDelegate,BypassDelegateOn,Push
  {
    vtkLog(TRACE, << "12. UseAsynchronousDelegate,BypassDelegateOn,Push");
    vtkNew<vtkMockVideoDecoder> decoder;
    vtkNew<vtkCompressedVideoPacket> packet;
    bool success = true;

    packet->SetSize(10);

    decoder->UseAsynchronousDelegate();
    decoder->BypassDelegateOn();
    decoder->Push(packet);
    decoder->Flush();
    success &= !decoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 12. UseAsynchronousDelegate,BypassDelegateOn,Push");
      return 1;
    }
  }

  return 0;
}
