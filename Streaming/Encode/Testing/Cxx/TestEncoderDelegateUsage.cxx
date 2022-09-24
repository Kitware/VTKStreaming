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

#include "vtkCPUVideoFrame.h"
#include "vtkIndent.h"
#include "vtkLogger.h"
#include "vtkMockVideoEncoder.h"
#include "vtkSmartPointer.h"

int TestEncoderDelegateUsage(int argc, char* argv[])
{
  // 1. UseAsynchronousDelegateOn
  {
    vtkLog(TRACE, << "1. UseAsynchronousDelegateOn");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->UseAsynchronousDelegateOn();
    success &= encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 1. UseAsynchronousDelegateOn");
      return 1;
    }
  }

  // 2. UseAsynchronousDelegateOff
  {
    vtkLog(TRACE, << "2. UseAsynchronousDelegateOff");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->UseAsynchronousDelegateOff();
    success &= !encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 2. UseAsynchronousDelegateOff");
      return 1;
    }
  }

  // 3. UseAsynchronousDelegateOn,Push,UseAsynchronousDelegateOff
  {
    vtkLog(TRACE, << "3. UseAsynchronousDelegateOn,Push,UseAsynchronousDelegateOff");
    vtkNew<vtkMockVideoEncoder> encoder;
    vtkNew<vtkCPUVideoFrame> frame;
    frame->SetWidth(4);
    frame->SetHeight(4);

    bool success = true;
    encoder->UseAsynchronousDelegateOn();
    success &= encoder->HasDelegate();

    encoder->Push(frame);
    success &= encoder->HasDelegate();

    encoder->UseAsynchronousDelegateOff();
    success &= !encoder->HasDelegate();

    encoder->Push(frame);
    success &= !encoder->HasDelegate();

    if (!success)
    {
      vtkLog(ERROR, << "Failed 3. UseAsynchronousDelegateOn,Push,UseAsynchronousDelegateOff");
      return 1;
    }
  }

  // 4. UseAsynchronousDelegateOff,Push,UseAsynchronousDelegateOn
  {
    vtkLog(TRACE, << "4. UseAsynchronousDelegateOff,Push,UseAsynchronousDelegateOn");
    vtkNew<vtkMockVideoEncoder> encoder;
    vtkNew<vtkCPUVideoFrame> frame;
    frame->SetWidth(4);
    frame->SetHeight(4);

    bool success = true;
    encoder->UseAsynchronousDelegateOff();
    success &= !encoder->HasDelegate();

    encoder->Push(frame);
    success &= !encoder->HasDelegate();

    encoder->UseAsynchronousDelegateOn();
    success &= encoder->HasDelegate();

    encoder->Push(frame);
    success &= encoder->HasDelegate();

    if (!success)
    {
      vtkLog(ERROR, << "Failed 4. UseAsynchronousDelegateOff,Push,UseAsynchronousDelegateOn");
      return 1;
    }
  }

  return 0;
}
