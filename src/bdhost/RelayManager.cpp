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

#include <algorithm>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <signal.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include <HttpConfig.h>
#include <BdSession.h>
#include <BdKademlia.h>
#include "Options.h"
#include "RelayManager.h"

namespace bdhost
{
  RelayManager::RelayManager(size_t limit)
    : limit(limit)
  {
    if (Options::relayExe.find("/") != std::string::npos)
    {
      this->relayPath = Options::relayExe;
    }
    else
    {
      char path[PATH_MAX];
      if (readlink("/proc/self/exe", path, PATH_MAX) > 0)
      {
        this->relayPath = std::string(dirname(path)) + "/" + Options::relayExe;
      }
      else
      {
        this->relayPath = Options::relayExe;
      }
    }
  }


  RelayManager::~RelayManager()
  {
    for (auto & entry : this->relays)
    {
      if (IsRelayAlive(entry.second))
      {
        kill(entry.second, SIGTERM);
      }
    }
  }


  void RelayManager::Validate()
  {
    this->Cleanup();

    if (this->relays.size() >= this->limit)
    {
      return;
    }

    bdfs::HttpConfig config;
    for(auto & kd : Options::kademlia)
    {
      auto session = bdfs::BdSession::CreateSession(kd.c_str(), &config);
      auto kademlia = std::static_pointer_cast<bdfs::BdKademlia>(session->CreateObject("Kademlia", "host://Kademlia", "Kademlia"));

      auto result = kademlia->QueryRelays(nullptr, this->limit);
      if (!result->Wait() || result->HasError())
      {
        continue;
      }

      auto & arr = result->GetResult();
      for (auto & info : arr)
      {
        if (this->relays.find(info.name) != this->relays.end())
        {
          continue;
        }

        int pid = this->StartRelayProcess(&info);
        if (pid != 0)
        {
          auto ptr = std::make_unique<bdfs::RelayInfo>();
          *ptr = std::move(info);
          this->relays[ptr->name] = pid;
          this->regs[ptr->name] = std::move(ptr);
        }
      }
      break;
    }
  }


  bool RelayManager::GetRelayInfos(std::vector<const bdfs::RelayInfo *> & output)
  {
    for (const auto & reg : this->regs)
    {
      output.push_back(reg.second.get());
    }

    return true;
  }


  bool RelayManager::IsRelayAlive(int pid)
  {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/exe", pid);

    char cmd[BUFSIZ];
    int len = readlink(path, cmd, sizeof(cmd) - 1);
    if (len <= 0)
    {
      return false;
    }

    cmd[len] = 0;

    if (!strstr(cmd, Options::relayExe.c_str()))
    {
      return false;
    }

    return true;
  }


  void RelayManager::Cleanup()
  {
    auto itr = this->relays.begin();
    while (itr != this->relays.end())
    {
      if (!IsRelayAlive(itr->second))
      {
        this->regs.erase(itr->first);
        this->relays.erase(itr++);
      }
      else
      {
        ++itr;
      }
    }
  }


  int RelayManager::StartRelayProcess(const bdfs::RelayInfo * info)
  {
    if (info->endpoints.empty())
    {
      return 0;
    }

    char portStr[16];
    snprintf(portStr, sizeof(portStr), "%u", info->endpoints[0].quicPort);

    char logFile[PATH_MAX];
    snprintf(logFile, sizeof(logFile), "/var/log/bdrelay.%s.log", Options::name.c_str());

    char * args[] = {
      const_cast<char *>(Options::relayExe.c_str()),
      const_cast<char *>("-n"),
      const_cast<char *>(Options::name.c_str()),
      const_cast<char *>("-h"),
      const_cast<char *>(info->endpoints[0].host.c_str()),
      const_cast<char *>("-p"),
      portStr,
      const_cast<char *>("-o"),
      logFile,
      nullptr
    };

    pid_t ppid = getpid();

    pid_t pid = fork();
    if (pid == 0)
    {
      int rtn = prctl(PR_SET_PDEATHSIG, SIGTERM);
      if (rtn == -1)
      {
        perror("Failed to listen on parent death signal.");
        exit(1);
      }

      if (getppid() != ppid)
      {
        exit(1);
      }

      execvp(Options::relayExe.c_str(), args);
      exit(0);
    }
    else if (pid < 0)
    {
      return 0;
    }
    else
    {
      return pid;      
    }
  }
}
