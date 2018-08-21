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
  uint16_t Options::port = 80;

  std::string Options::name;

  std::string Options::endpoint;

  std::string Options::kademlia = "http://localhost:7800";

  std::string Options::repo = ".";

  uint64_t Options::size = 1LLU * 1024 * 1024 * 1024; // 1GB

  std::string Options::relayExe = "./relay-client";

  size_t Options::maxRelayCount = 0;


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
    printf("  -n <name>       name of the host\n");
    printf("  -p <port>       port to listen on (default:80)\n");
    printf("  -e <url>        endpoint url to register (default:http://localhost)\n");
    printf("  -k <kad>        kademlia service url (default:http://localhost:7800)\n");
    printf("  -s <size>       storage size to publish in bytes (default: 1073741824)\n");
    printf("  -a <relay_exe>  relay executable path (default:relay-client)\n");
    printf("  -m <max_relays> maximum relay count (default:0)\n");
    printf("\n");
    exit(message == NULL ? 0 : 1);
  }


  bool Options::Init(int argc, const char ** argv)
  {
    #define assert_argument_index(_idx, _name) \
      if ((_idx) >= argc) { Usage("Missing argument '%s'.\n", _name); }

    for (int i = 1; i < argc; ++i)
    {
      if (strcmp(argv[i], "-s") == 0)
      {
        assert_argument_index(++i, "size");
        size = static_cast<uint64_t>(strtoull(argv[i], nullptr, 10));
        if (size < 1024 * 1024)
        {
          Usage("size is invalid or too small: %llu\n", (unsigned long long)size);
        }
      }
      else if (strcmp(argv[i], "-n") == 0)
      {
        assert_argument_index(++i, "name");
        name = argv[i];
      }
      else if (strcmp(argv[i], "-e") == 0)
      {
        assert_argument_index(++i, "url");
        endpoint = argv[i];
      }
      else if (strcmp(argv[i], "-k") == 0)
      {
        assert_argument_index(++i, "kad");
        kademlia = argv[i];
      }
      else if (strcmp(argv[i], "-a") == 0)
      {
        assert_argument_index(++i, "relay_exe");
        relayExe = argv[i];
      }
      else if (strcmp(argv[i], "-p") == 0)
      {
        assert_argument_index(++i, "port");
        port = static_cast<uint16_t>(atoi(argv[i]));
      }
      else if (strcmp(argv[i], "-m") == 0)
      {
        assert_argument_index(++i, "max_relays");
        maxRelayCount = static_cast<size_t>(atoi(argv[i]));
      }
      else
      {
        Usage("Error: unknown argument '%s'.\n", argv[i]);
      }
    }

    return true;
  }
}
