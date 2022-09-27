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