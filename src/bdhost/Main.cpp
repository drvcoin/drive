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
#include <thread>
#include <mongoose.h>
#include <unistd.h>
#include "Options.h"
#include "HttpModule.h"
#include "HttpServer.h"
#include "HttpConfig.h"
#include "BdSession.h"
#include "BdKademlia.h"
#include "RelayManager.h"
#include "HostInfo.h"
#include "Global.h"

/*
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
*/


static void init_kad()
{
  auto thread = std::thread(
    []()
    {
      auto key = "ep:" + bdhost::Options::name;

      bdfs::HostInfo hostInfo;
      hostInfo.url = bdhost::Options::endpoint;
      if (hostInfo.url.empty())
      {
        char buf[BUFSIZ];
        sprintf(buf, "http://localhost:%u", bdhost::Options::port);
        hostInfo.url = buf;
      }

      bdhost::RelayManager relayManager{bdhost::Options::maxRelayCount};

      while (true)
      {
        relayManager.Validate();

        std::vector<const bdfs::RelayInfo *> relays;
        if (!relayManager.GetRelayInfos(relays))
        {
          printf("WARNING: failed to find any relays.\n");
        }

        hostInfo.relays.clear();

        for (auto ptr : relays)
        {
          hostInfo.relays.push_back(*ptr);
          hostInfo.relays[hostInfo.relays.size() - 1].name = bdhost::Options::name;
        }

        auto host = hostInfo.ToString();
        printf("Host info: %s\n", host.c_str());

        bdfs::HttpConfig config;
        auto session = bdfs::BdSession::CreateSession(bdhost::Options::kademlia.c_str(), &config); 

        auto kademlia = std::static_pointer_cast<bdfs::BdKademlia>(session->CreateObject("Kademlia", "host://Kademlia", "Kademlia"));

        auto result = kademlia->SetValue(key.c_str(), host.c_str(), host.size(), time(nullptr), 7 * 24 * 60 * 60);

        if (!result->Wait() || !result->GetResult())
        {
          printf("WARNING: failed to publish endpoints to kademlia\n");
        }

        // Publish size and reputaiton of host
        auto contract = bdhost::g_contracts->LoadContract(bdhost::Options::contract.c_str());

        auto result_query = kademlia->PublishStorage(bdhost::Options::name.c_str(), bdhost::Options::contract.c_str(), contract->Size()/(1024*1024), contract->Reputation());

        if (!result_query->Wait() || !result_query->GetResult())
        {
          printf("WARNING: failed to publish storage and reputation to kademlia\n");
        }

        sleep(24 * 60 * 60);
      }
    }
  );

  thread.detach();
}


int main(int argc, const char ** argv)
{
  bdhost::Options::Init(argc, argv);

  init_kad();

  bdhost::g_contracts = new bdcontract::ContractRepository(bdhost::Options::repo.c_str());

  bdhttp::HttpModule::Initialize();

  bdhttp::HttpServer server;

  if (!server.Start(bdhost::Options::port, nullptr))
  {
    printf("[Main]: failed to start http server on port %u.\n", bdhost::Options::port);
    return -1;
  }

  while (server.IsRunning())
  {
    sleep(1);
  }

  bdhttp::HttpModule::Stop();

  return 0;
}
