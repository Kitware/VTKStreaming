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
#include "vtkOpenGLVideoFrame.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingTestUtility.h"

int TestEncoderDelegateUsage(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  // 1. AsyncModeOn
  {
    vtkLog(TRACE, << "1. AsyncModeOn");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->AsyncModeOn();
    success &= encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 1. AsyncModeOn");
      return 1;
    }
  }

  // 2. AsyncModeOff
  {
    vtkLog(TRACE, << "2. AsyncModeOff");
    vtkNew<vtkMockVideoEncoder> encoder;
    bool success = true;

    encoder->AsyncModeOff();
    success &= !encoder->HasDelegate();
    if (!success)
    {
      vtkLog(ERROR, << "Failed 2. AsyncModeOff");
      return 1;
    }
  }

  // 3. AsyncModeOn,Push,AsyncModeOff
  {
    vtkLog(TRACE, << "3. AsyncModeOn,Push,AsyncModeOff");
    vtkNew<vtkMockVideoEncoder> encoder;
    vtkNew<vtkOpenGLVideoFrame> frame;
    frame->SetWidth(4);
    frame->SetHeight(4);
    frame->ComputeDefaultStrides();
    frame->AllocateDataStore();

    bool success = true;
    encoder->AsyncModeOn();
    success &= encoder->HasDelegate();

    encoder->Push(frame);
    success &= encoder->HasDelegate();

    encoder->AsyncModeOff();
    success &= !encoder->HasDelegate();

    encoder->Push(frame);
    success &= !encoder->HasDelegate();

    if (!success)
    {
      vtkLog(ERROR, << "Failed 3. AsyncModeOn,Push,AsyncModeOff");
      return 1;
    }
  }

  // 4. AsyncModeOff,Push,AsyncModeOn
  {
    vtkLog(TRACE, << "4. AsyncModeOff,Push,AsyncModeOn");
    vtkNew<vtkMockVideoEncoder> encoder;
    vtkNew<vtkOpenGLVideoFrame> frame;
    frame->SetWidth(4);
    frame->SetHeight(4);
    frame->ComputeDefaultStrides();
    frame->AllocateDataStore();

    bool success = true;
    encoder->AsyncModeOff();
    success &= !encoder->HasDelegate();

    encoder->Push(frame);
    success &= !encoder->HasDelegate();

    encoder->AsyncModeOn();
    success &= encoder->HasDelegate();

    encoder->Push(frame);
    success &= encoder->HasDelegate();

    if (!success)
    {
      vtkLog(ERROR, << "Failed 4. AsyncModeOff,Push,AsyncModeOn");
      return 1;
    }
  }

  return 0;
}
