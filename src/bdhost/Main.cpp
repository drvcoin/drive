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

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <mongoose.h>
#include <unistd.h>
#include "Options.h"
#include "HttpModule.h"
#include "HttpServer.h"

static int Listen()
{
  bdhost::HttpModule::Initialize();

  bdhost::HttpServer server;

  if (!server.Start(bdhost::Options::port, nullptr))
  {
    printf("[Main]: failed to start http server on port %u.\n", bdhost::Options::port);
    return -1;
  }

  while (server.IsRunning())
  {
    sleep(1);
  }

  bdhost::HttpModule::Stop();

  return 0;
}


static int Init(const std::string & name)
{
  mkdir(name.c_str(), 0755);

  FILE * config = fopen((name + "/.config").c_str(), "w");
  if (!config)
  {
    return -1;
  }

  uint64_t blockCount = 256;
  uint64_t blockSize = 1*1024*1024;

  fwrite(&blockCount, 1, sizeof(blockCount), config);
  fwrite(&blockSize, 1, sizeof(blockSize), config);

  fclose(config);

  return 0;
}


int main(int argc, const char ** argv)
{
  bdhost::Options::Init(argc, argv);

  if (bdhost::Options::action == "init")
  {
    return Init(bdhost::Options::name);
  }
  else if (bdhost::Options::action == "listen")
  {
    return Listen();
  }
}
