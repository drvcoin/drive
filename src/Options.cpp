/*
  Copyright (c) 2018 Drive Foundation

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "Options.h"

#include <string.h>
#include <stdarg.h>

namespace dfs
{
  Action::T Options::Action = Action::Unknown;
  uint16_t Options::DataBlocks = 6;
  uint16_t Options::CodeBlocks = 2;
  std::vector<std::string> Options::Paths;

  extern void Exit(const char * format, ...);

  void Usage(const char * format = NULL, ...)
  {
    if (format != NULL)
    {
      va_list args;
      va_start(args, format);
      vprintf(format, args);
      va_end(args);
      printf("\n");
    }

    printf("Usage: drive {action} [options] [files]\n");
    printf("\n");
    printf("Actions: create,delete,verify,resize,mount,unmount,daemon\n");
    printf("\n");
    printf("Options: create\n");
    printf("\n");
    printf("  -p {path}      Path\n");
    printf("  -d {blocks}    Data blocks (1 .. 255)\n");
    printf("  -c {blocks}    Code blocks (1 .. 255)\n");
    printf("  -?|h           Show this help screen\n");
    printf("\n");

    exit(format == NULL ? 0 : 1);
  }

  void Options::Init(int argc, char ** argv)
  {
    for (int i = 1; i < argc; i++)
    {
      char * arg = argv[i];

      if (strcmp(arg, "-?") == 0 || strcmp(arg, "-h") == 0)
      {
        Usage();
      }
      else if (strcmp(arg, "-d") == 0)
      {
        int val = atoi(argv[++i]);
        if (val < 1 || val > 256)
        {
          Usage("\nError: Invalid data block count: %d (valid: 1 .. 255)\n", val);
        }
        Options::DataBlocks = (uint16_t)val;
      }
      else if (strcmp(arg, "-c") == 0)
      {
        int val = atoi(argv[++i]);
        if (val < 1 || val > 256)
        {
          Usage("\nError: Invalid code block count: %d (valid: 1 .. 255)\n", val);
        }
        Options::CodeBlocks = (uint16_t)val;
      }
      else if (arg[0] == '-')
      {
        Usage("\nError: Unrecognized option: %s\n", arg);
      }
      else
      {
        if (Options::Action == Action::Unknown)
        {
          Options::Action = Action::FromString(arg);
          if (Options::Action == Action::Unknown)
          {
            Usage("\nError: Unknown action: %s\n", argv[i]);
          }
        }
        else
        {
          Options::Paths.push_back(arg);
        }
      }
    }

    if (Options::Action == Action::Unknown)
    {
      Usage("\nError: No action specified\n");
    }

    uint16_t totalBlocks = Options::DataBlocks + Options::CodeBlocks;
    if (totalBlocks > 256)
    {
      Usage("\nError: Too many blocks specified: %d (valid: datablocks+codeblocks <= 256)\n", totalBlocks);
    }

    if (Paths.size() == 0)
    {
      Usage("\nError: No files specified\n");
    }
  }
}
