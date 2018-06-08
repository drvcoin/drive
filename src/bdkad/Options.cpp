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


#include <arpa/inet.h>


namespace bdhost
{
  uint16_t Options::port = 80;

  std::string Options::repo = ".";

  const char * Options::k_root;

  uint32_t Options::k_addr;
  uint16_t Options::k_port;

  uint32_t Options::k_bootaddr;
  uint16_t Options::k_bootport;


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

    printf("Usage: bdhost [option]\n");
    printf("\n");
    printf("Options:\n");
    printf("  -p <port>       port to listen on (default:80)\n");
    printf("  -r <path>       root path of contract repository (default:.)\n");
    printf("\n");
    exit(message == NULL ? 0 : 1);
  }


  bool Options::Init(int argc, const char ** argv)
  {
    #define assert_argument_index(_idx, _name) \
      if ((_idx) >= argc) { Usage("Missing argument '%s'.\n", _name); }

    for (int i = 1; i < argc; ++i)
    {
      if (strcmp(argv[i], "-r") == 0)
      {
        assert_argument_index(++i, "path");
        repo = argv[i];
      }
      else if (strcmp(argv[i], "-p") == 0)
      {
        assert_argument_index(++i, "port");
        port = static_cast<uint16_t>(atoi(argv[i]));
      }
      else if (strcmp(argv[i], "-k") == 0)
      {
        k_root = argv[++i];

        k_addr = static_cast<uint32_t>(inet_addr(argv[++i]));
        k_port = static_cast<uint16_t>(atoi(argv[++i]));

        k_bootaddr = static_cast<uint32_t>(inet_addr(argv[++i]));
        k_bootport = static_cast<uint16_t>(atoi(argv[++i]));
      }
      else
      {
        Usage("Error: unknown argument '%s'.\n", argv[i]);
      }
    }

    return true;
  }
}
