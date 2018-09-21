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
#include <streambuf>
#include <fstream>
#include <array>

#include <thread>
#include "cm256.h"
#include "UnixDomainSocket.h"
#include "ClientManager.h"
#include "VolumeManager.h"
#include "ActionHandler.h"
#include "Util.h"

using namespace dfs;
using namespace bdfs;

ClientManager client;
sigset_t mySigs;

void setupSigHandler()
{
  sigemptyset(&mySigs);
  sigaddset(&mySigs, SIGINT);
  sigaddset(&mySigs, SIGTERM);

  pthread_sigmask(SIG_BLOCK, &mySigs, NULL);

  std::thread([]{
    int sig;
    sigwait(&mySigs, &sig);
    printf("Interrupted by signal (%d)\n", sig);
    ActionHandler::Cleanup();
    client.Stop();
  }).detach();
}

int main(int argc, char * * argv)
{
  setupSigHandler();

  std::ifstream cfg("/etc/drive/bdfs.conf");
  std::string data((std::istreambuf_iterator<char>(cfg)),
               std::istreambuf_iterator<char>());

  if(!data.size())
  {
    return 0;
  }

  Json::Reader reader;
  Json::Value json;
  if (!reader.parse(data.c_str(), data.size(), json, false) ||
      !json.isObject())
  {
    return 0;
  }

  if(json["kademlia"].isArray())
  {
    for(int i = 0; i < json["kademlia"].size(); i++)
    {
      auto kd = json["kademlia"][i].asString();
      VolumeManager::kademliaUrl.emplace_back(kd);
    }
  }

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

  client.Run();

  VolumeManager::Stop();

  printf("bdfsclient server exiting...\n");
  return 0;
}
