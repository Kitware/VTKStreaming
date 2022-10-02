/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamingTestUtility.cxx

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkStreamingTestUtility.h"
#include "vtkLogger.h"
#include "vtkUnsignedCharArray.h"

#include <cstdlib>
#include <string>

// Problem: vtkLogger eats up verbosity and it doesn't respect command line verbosity.
// Solution: We use -V token to specify verbosity from the CLI.
void vtkStreamingTestUtility::SetLoggerVerbosityFromCli(int argc, char** argv)
{
  for (int i = 0; i < argc; ++i)
  {
    const char dash_token = '-';
    const char verb_token = 'V';
    int token_id = 0;
    if (std::string(argv[i])[token_id++] == dash_token &&
      std::string(argv[i])[token_id++] == verb_token)
    {
      auto value = std::string(argv[i])[token_id];
      std::string verbosity_level_str(&value, 1);
      auto verbosity_level = vtkLogger::ConvertToVerbosity(std::atoi(verbosity_level_str.c_str()));
      vtkLogger::SetStderrVerbosity(verbosity_level);
    }
  }
}

bool vtkStreamingTestUtility::GetInteractive(int argc, char** argv)
{
  for (int i = 0; i < argc; ++i)
  {
    const char dash_token = '-';
    const char verb_token = 'I';
    int token_id = 0;
    if (std::string(argv[i])[token_id++] == dash_token &&
      std::string(argv[i])[token_id++] == verb_token)
    {
      return true;
    }
  }
  return false;
}

vtkUnsignedCharArray* vtkStreamingTestUtility::GenerateRGBA32ColorBars(
  int width, int height, int shift /*=0*/)
{
  auto result = vtkUnsignedCharArray::New();
  int ndivs = (width + 7) >> 3;
  std::vector<unsigned char> reds(8), greens(8), blues(8), alphas(8, 255);
  reds = { 255, 255, 0, 0, 255, 255, 0, 0 };
  greens = { 255, 255, 255, 255, 0, 0, 0, 0 };
  blues = { 255, 0, 255, 0, 255, 0, 255, 0 };
  for (int j = 0; j<height>> 1; ++j)
  {
    for (int i = 0; i < width; ++i)
    {
      const int bar = ((i / ndivs) + (shift % 8)) % 8;
      result->InsertNextValue(reds[bar]);
      result->InsertNextValue(greens[bar]);
      result->InsertNextValue(blues[bar]);
      result->InsertNextValue(alphas[bar]);
    }
  }
  for (int j = height >> 1; j < height; ++j)
  {
    for (int i = 0; i < width; ++i)
    {
      const int bar = (7 - (i / ndivs) + (shift % 8)) % 8;
      result->InsertNextValue(reds[bar]);
      result->InsertNextValue(greens[bar]);
      result->InsertNextValue(blues[bar]);
      result->InsertNextValue(alphas[bar]);
    }
  }
  return result;
}
