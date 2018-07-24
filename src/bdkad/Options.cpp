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
#include <fstream>

#include <arpa/inet.h>
#include <json/json.h>

namespace bdhost
{
  uint16_t Options::port = 80;

  std::string  Options::k_root;

  uint32_t Options::k_addr;
  uint16_t Options::k_port;


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

    printf("Usage: bdkad [option]\n");
    printf("\n");
    printf("Options:\n");
    printf("  -c <config file>      config file in json format\n");
    printf("  -p <port>             port to listen http request on (default:80)\n");
    printf("  -k <path>             kademlia node name and path\n");
    printf("  -n <addr> <port>      kademlia node address and port\n");
    printf("\n");
    exit(message == NULL ? 0 : 1);
  }


  bool Options::Init(int argc, const char ** argv)
  {
    #define assert_argument_index(_idx, _name) \
      if ((_idx) >= argc) { Usage("Missing argument '%s'.\n", _name); }

    for (int i = 1; i < argc; ++i)
    {
      if (strcmp(argv[i], "-c") == 0)
      {
        assert_argument_index(++i, "config");
        LoadConfig(argv[i]);
      }
      else if (strcmp(argv[i], "-p") == 0)
      {
        assert_argument_index(++i, "port");
        port = static_cast<uint16_t>(atoi(argv[i]));
      }
      else if (strcmp(argv[i], "-k") == 0)
      {
        assert_argument_index(++i, "path");
        k_root = argv[i];
      }
      else if (strcmp(argv[i], "-n") == 0)
      {
        assert_argument_index(++i, "node addr");
        k_addr = static_cast<uint32_t>(inet_addr(argv[i]));
        assert_argument_index(++i, "node port");
        k_port = static_cast<uint16_t>(atoi(argv[i]));

      }
      else
      {
        Usage("Error: unknown argument '%s'.\n", argv[i]);
      }
    }

    return true;
  }

  bool Options::LoadConfig(const char * path)
  {
    std::ifstream ifs(path);

    std::string content( (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>() );


    Json::Reader reader;
    Json::Value json;

    if (!reader.parse(content, json, false) ||
        !json.isObject() ||
        !json["http_port"].isIntegral() ||
        !json["node_name"].isString() ||
        !json["kad_addr"].isString() ||
        !json["kad_port"].isIntegral())
    {
      printf("ERROR reading config file %s\n",path);
      return false;
    }

    port = static_cast<uint16_t>( json["http_port"].asUInt() );
    k_root = json["node_name"].asString();

    k_addr = static_cast<uint32_t>( inet_addr( json["kad_addr"].asString().c_str() ));
    k_port = static_cast<uint16_t>( json["kad_port"].asUInt() );

    return true;
  }

}
