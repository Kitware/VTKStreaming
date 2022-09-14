/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestEncoderDelegateUsage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises the API of vtkAbstractEncoder when it is instructed
// to bypass the processing delegate.
// The aim is to verify that Push/GetResult do not use a delegate.

#include "vtkIndent.h"
#include "vtkLogger.h"
#include "vtkMockVideoEncoder.h"
#include "vtkCPUVideoFrame.h"
#include "vtkSmartPointer.h"
#include <cstdlib>

int TestEncoderDelegateUsage(int argc, char* argv[])
{
  // 1. BypassDelegateOff,UseSynchronousDelegate
  {
    vtkLog(TRACE, << "1. BypassDelegateOff,UseSynchronousDelegate");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->BypassDelegateOff();
    encoder->UseSynchronousDelegate();
    success &= encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 1. BypassDelegateOff,UseSynchronousDelegate");
      return 1;
    }
  }

  // 2. BypassDelegateOn,UseSynchronousDelegate
  {
    vtkLog(TRACE, << "2. BypassDelegateOn,UseSynchronousDelegate");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->BypassDelegateOn();
    encoder->UseSynchronousDelegate();
    success &= !encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 2. BypassDelegateOn,UseSynchronousDelegate");
      return 1;
    }
  }

  // 3. UseSynchronousDelegate,BypassDelegateOff
  {
    vtkLog(TRACE, << "3. UseSynchronousDelegate,BypassDelegateOff");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->UseSynchronousDelegate();
    encoder->BypassDelegateOff();
    success &= encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 3. UseSynchronousDelegate,BypassDelegateOff");
      return 1;
    }
  }

  // 4. UseSynchronousDelegate,BypassDelegateOn
  {
    vtkLog(TRACE, << "4. UseSynchronousDelegate,BypassDelegateOff");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->UseSynchronousDelegate();
    encoder->BypassDelegateOn();
    success &= encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 4. UseSynchronousDelegate,BypassDelegateOff");
      return 1;
    }
  }

  // 5. UseSynchronousDelegate,BypassDelegateOff,Push
  {
    vtkLog(TRACE, << "5. UseSynchronousDelegate,BypassDelegateOff,Push");
    vtkNew<vtkMockVideoEncoder> encoder;
    vtkNew<vtkCPUVideoFrame> frame;
    bool success = true;

    frame->SetWidth(4);
    frame->SetHeight(4);
    frame->AllocateDataStore();

    encoder->UseSynchronousDelegate();
    encoder->BypassDelegateOff();
    encoder->Push(frame);
    encoder->Flush();
    success &= encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 5. UseSynchronousDelegate,BypassDelegateOff,Push");
      return 1;
    }
  }

  // 6. UseSynchronousDelegate,BypassDelegateOn,Push
  {
    vtkLog(TRACE, << "6. UseSynchronousDelegate,BypassDelegateOn,Push");
    vtkNew<vtkMockVideoEncoder> encoder;
    vtkNew<vtkCPUVideoFrame> frame;
    bool success = true;

    frame->SetWidth(4);
    frame->SetHeight(4);
    frame->AllocateDataStore();

    encoder->UseSynchronousDelegate();
    encoder->BypassDelegateOn();
    encoder->Push(frame);
    encoder->Flush();
    success &= !encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 6. UseSynchronousDelegate,BypassDelegateOn,Push");
      return 1;
    }
  }

  // 7. BypassDelegateOff,UseAsynchronousDelegate
  {
    vtkLog(TRACE, << "7. BypassDelegateOff,UseAsynchronousDelegate");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->BypassDelegateOff();
    encoder->UseAsynchronousDelegate();
    success &= encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 7. BypassDelegateOff,UseAsynchronousDelegate");
      return 1;
    }
  }

  // 8. BypassDelegateOn,UseAsynchronousDelegate
  {
    vtkLog(TRACE, << "8. BypassDelegateOn,UseAsynchronousDelegate");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->BypassDelegateOn();
    encoder->UseAsynchronousDelegate();
    success &= !encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 8. BypassDelegateOn,UseAsynchronousDelegate");
      return 1;
    }
  }

  // 9. UseAsynchronousDelegate,BypassDelegateOff
  {
    vtkLog(TRACE, << "9. UseAsynchronousDelegate,BypassDelegateOff");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->UseAsynchronousDelegate();
    encoder->BypassDelegateOff();
    success &= encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 9. UseAsynchronousDelegate,BypassDelegateOff");
      return 1;
    }
  }

  // 10. UseAsynchronousDelegate,BypassDelegateOn
  {
    vtkLog(TRACE, << "10. UseAsynchronousDelegate,BypassDelegateOn");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->UseAsynchronousDelegate();
    encoder->BypassDelegateOn();
    success &= encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 10. UseAsynchronousDelegate,BypassDelegateOn");
      return 1;
    }
  }

  // 11. UseAsynchronousDelegate,BypassDelegateOff,Push
  {
    vtkLog(TRACE, << "11. UseAsynchronousDelegate,BypassDelegateOff,Push");
    vtkNew<vtkMockVideoEncoder> encoder;
    vtkNew<vtkCPUVideoFrame> frame;
    bool success = true;

    frame->SetWidth(4);
    frame->SetHeight(4);
    frame->AllocateDataStore();

    encoder->UseAsynchronousDelegate();
    encoder->BypassDelegateOff();
    encoder->Push(frame);
    encoder->Flush();
    success &= encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 11. UseAsynchronousDelegate,BypassDelegateOff,Push");
      return 1;
    }
  }

  // 12. UseAsynchronousDelegate,BypassDelegateOn,Push
  {
    vtkLog(TRACE, << "12. UseAsynchronousDelegate,BypassDelegateOn,Push");
    vtkNew<vtkMockVideoEncoder> encoder;
    vtkNew<vtkCPUVideoFrame> frame;
    bool success = true;

    frame->SetWidth(4);
    frame->SetHeight(4);
    frame->AllocateDataStore();

    encoder->UseAsynchronousDelegate();
    encoder->BypassDelegateOn();
    encoder->Push(frame);
    encoder->Flush();
    success &= !encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 12. UseAsynchronousDelegate,BypassDelegateOn,Push");
      return 1;
    }
  }

  return 0;
}
