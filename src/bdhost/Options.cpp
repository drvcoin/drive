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
#include <stdio.h>
#include <stdlib.h>

namespace bdhost
{
  std::string Options::action;

  std::string Options::name;

  uint16_t Options::port;


  bool Options::Usage(const char * message, ...)
  {
    if (message != NULL)
    {
      va_list args;
      va_start(args, message);
      vprintf(message, args);
      va_end(args);
      printf("\n");
    }

    printf("Usage: bdhost {action}\n");
    printf("\n");
    printf("Actions:\n");
    printf("\n");
    printf("  init <name>     Initialize partition with blockCount=256 and blockSize=1024*1024\n");
    printf("  listen <port>   Listen on the specified port\n");
    printf("\n");
    exit(message == NULL ? 0 : 1);
  }


  bool Options::Init(int argc, const char ** argv)
  {
    if (argc < 2)
    {
      Usage("Error: missing action\n");
    }

    action = argv[1];

    if (action == "listen")
    {
      if (argc < 3)
      {
        Usage("Error: missing port\n");
      }

      port = static_cast<uint16_t>(atoi(argv[2]));
    }
    else if (action == "init")
    {
      if (argc < 3)
      {
        Usage("Error: missing name\n");
      }

      name = argv[2];
    }
    else
    {
      Usage("Error: unknown action\n");
    }

    return true;
  }
}
