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

namespace bdcontract
{
  std::string Options::action;
  std::string Options::repo;
  std::string Options::name;
  std::string Options::consumer;
  std::string Options::provider;
  uint64_t Options::size;


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

    printf("Usage: bdcontract {action} [options]\n");
    printf("\n");
    printf("Actions:\n");
    printf("\n");
    printf("  create <name> <consumer> <provider> <size>\n");
    printf("    Create a new contract.\n");
    printf("    Args:\n");
    printf("      name:     contract name\n");
    printf("      consumer: consumer name\n");
    printf("      provider: provider base URL\n");
    printf("      size:     total size in MB\n");
    printf("\n");
    printf("  list\n");
    printf("    List all the contracts.\n");
    printf("\n");
    printf("  show <name>\n");
    printf("    Show the content of the contract.\n");
    printf("    Args:\n");
    printf("      name:     contract name\n");
    printf("\n");
    printf("  delete <name>\n");
    printf("    Delete the contract.\n");
    printf("    Args:\n");
    printf("      name:     contract name\n");
    printf("\n");
    printf("Options\n");
    printf("  -r <path>     root path for contract repository\n");
    printf("\n");
    exit(message == NULL ? 0 : 1);
  }

 
  static inline void assert_argument_index(int idx, int argc)
  {
    if (idx >= argc)
    {
      
    }
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
      else
      {
        action = argv[i];

        if (action != "list")
        {
          assert_argument_index(++i, "name");
          name = argv[i];
          
          if (action == "create")
          {
            assert_argument_index(++i, "consumer");
            consumer = argv[i];

            assert_argument_index(++i, "provider");
            provider = argv[i];

            assert_argument_index(++i, "size");
            errno = 0;
            size = strtoull(argv[i], nullptr, 10) * 1024 * 1024;

            if (errno)
            {
              Usage("Invalid size.\n");
            }
          }
          else if (action != "show" && action != "delete")
          {
            Usage("Unknown action.\n");
          }
        }
      }
    }

    return true;
  }
}
