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

#include <stdio.h>
#include <signal.h>
#include <sstream>

#include "cm256.h"
#include "UnixDomainSocket.h"
#include "ClientManager.h"
#include "VolumeManager.h"
#include "ActionHandler.h"
#include "Util.h"

using namespace dfs;
using namespace bdfs;

ClientManager client;

void signalHandler(int signum)
{
  printf("Interrupt received by signal ( %d ).\n",signum);
  ActionHandler::Cleanup();
  client.Stop();
  exit(signum);
}

int main(int argc, char * * argv)
{
  if (argc > 1)
  {
    VolumeManager::kademliaUrl.push_back(argv[1]);
  }

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  
  if (cm256_init()) {
    exit(1);
  }
  
	auto nbdPaths = execCmd("ls -1 /dev/nbd*");
  std::stringstream ss;
  ss.str(nbdPaths);
  std::string path;
  while(std::getline(ss,path,'\n') && !path.empty())
  {
    ActionHandler::AddNbdPath(path);
  }
  
  if(!client.Start())
  {
    printf("Error: Failed to start processing thread.\n");
  }

  while(true)
  {
    sleep(1);
  }

  client.Stop();

  return 0;
}
