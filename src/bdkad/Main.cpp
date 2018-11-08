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
#include <drive/httpserver/HttpModule.h>
#include <drive/httpserver/HttpServer.h>
#include "Options.h"
#include "KademliaModule.h"



int main(int argc, const char ** argv)
{
  bdkad::Options::Init(argc, argv);

  bdhttp::HttpModule::Initialize();

  bdhttp::HttpServer server;

  if (!server.Start(bdkad::Options::http_port, nullptr))
  {
    printf("[Main]: failed to start http server on port %u.\n", bdkad::Options::http_port);
    return -1;
  }

  bdkad::KademliaModule::Initialize();

  while (server.IsRunning())
  {
    sleep(1);
  }

  bdhttp::HttpModule::Stop();

  return 0;
}
