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
#include <sys/stat.h>
#include <cmath>
#include <limits>
#include <vector>
#include <json/json.h>

#include "BdTypes.h"
#include "HttpConfig.h"
#include "BdSession.h"

#include "Options.h"
#include "Util.h"
#include "VolumeManager.h"
#include "Volume.h"
#include "BdPartitionFolder.h"
#include "Buffer.h"
#include "ContractRepository.h"
#include "Cache.h"

#include "CrossIPC.h"
#include "BdProtocol.h"

using namespace dfs;
using namespace bdfs;

static bdfs::HttpConfig defaultConfig;

#define SENDRECV_TIMEOUT 30
#define MAX_RECV_TIMEOUT 300


std::unique_ptr<char[]> ReceiveBdcp(CrossIPC *ipc)
{
  uint32_t length;
  uint32_t cbLen;
  if (ipc == nullptr)
  {
    return nullptr;
  }

  ipc->Read((uint8_t*)&length, sizeof(uint32_t), cbLen);
  std::unique_ptr<char[]>buff = std::make_unique<char[]>(length);
  ((bdcp::BdHdr*)buff.get())->length = length;

  ipc->Read((uint8_t*)buff.get() + sizeof(uint32_t), length - sizeof(uint32_t), cbLen);

  return buff;
}

std::unique_ptr<char[]> SendReceive(const std::vector<std::string> &args, bdcp::T type)
{
  CrossIPC ipc;
  auto buff = bdcp::Create(type, args);
  auto buffLength = ((bdcp::BdHdr*)buff.get())->length;
  std::unique_ptr<char[]> returnBuff = nullptr;

  if (!ipc.Attach())
  {
    printf("Error: Unable to connect to bdfsclient daemon.\n");
    exit(0);
  }
  else if (!ipc.Write((uint8_t*)buff.get(), buffLength))
  {
    printf("Error: Unable to sendmessage to bdfsclient daemon.\n");
    exit(0);
  }
  else
  {
    returnBuff = ReceiveBdcp(&ipc);
    if (buff == nullptr)
    {
      printf("Error: Unable to readresponse from bdfsclient daemon.\n");
      exit(0);
    }
  }
  ipc.Disconnect();
  return returnBuff;
}


void ListVolumes()
{
  CrossIPC ipc;
  auto buff = bdcp::Create(bdcp::QUERY_VOLUMEINFO, std::vector<std::string>());
  auto buffLength = ((bdcp::BdHdr*)buff.get())->length;
  
  if(!ipc.Attach())
  {
    printf("Error: Unable to connect to bdfsclient daemon.\n");
    exit(0);
  }
  else if (!ipc.Write((uint8_t*)buff.get(), buffLength))
  {
    printf("Error: Unable to sendmessage to bdfsclient daemon.\n");
    exit(0);
  }
  else
  {
#if !defined(_WIN32)
    printf("\n%-25s%-20s%-25s\n","VOLUME NAME", "NBD PATH", "MOUNT PATH");
#endif
    do
    {
      std::unique_ptr<char[]> resp = ReceiveBdcp(&ipc);
      if(resp == nullptr || ((bdcp::BdResponse*)resp.get())->status == 0)
      {
        break;
      }

      std::vector<std::string> args = bdcp::Parse(resp);
#if defined(_WIN32)
      if (args.size() == 2)
      {
        printf("Volume name: %s\n", args[0].c_str());
        std::string cmd = "imdisk -l -m " + args[1];
        system(cmd.c_str());
      }
      printf("\n");
#else
      for(auto &arg : args)
      {
        printf("%-25s", arg.c_str());
      }
      printf("\n");
#endif
    } while(true);
  }
}


void HandleOptions()
{
  std::vector<std::string> args;

  switch(Options::Action)
  {
    case Action::Mount:
    {
      if(Options::Paths.size() != 1)
      {
        printf("Missing <volumename> or <path>\n");
      }
      else
      {
        args.emplace_back(Options::Name);
        args.emplace_back(Options::Paths[0]);
        auto resp = SendReceive(args,bdcp::BIND);
        auto respParams = bdcp::Parse(resp);

        if(!((bdcp::BdResponse*)resp.get())->status || respParams.size() != 1)
        {
          printf("Bind failed for volume '%s'.\n",Options::Name.c_str());
          exit(0);
        }

#if !defined(_WIN32)
        int timeout = 10;	
        while(!nbd_ready(respParams[0].c_str()) && timeout--)
        {
          sleep(1);
        }

        if(timeout == 0)
        {
          printf("Bind timeout for volume '%s'.\n",Options::Name.c_str());
          exit(0);
        }
#endif
        printf("Mounting '%s' to '%s'\n",respParams[0].c_str(),Options::Paths[0].c_str());

#if defined(_WIN32)
        std::string cmd = "imdisk -a -t proxy -o shm -f " + respParams[0] + " -m " + Options::Paths[0];
        for (auto arg : Options::ExternalArgs)
        {
          cmd += " " + arg;
        }
        system(cmd.c_str());
#else
        std::string user = execCmd("id -u -n");
        user = user.substr(0,user.size()-1);
        std::string group = execCmd("id -g -n");
        group = group.substr(0,group.size()-1);
        std::string chownCmd = "sudo chown " + user + ":" + group + " " + Options::Paths[0];
        
        std::string cmd = "sudo mount";
        for(auto arg : Options::ExternalArgs)
        {
          cmd += " " + arg;
        }
        cmd += " " + respParams[0];
        cmd += " " + Options::Paths[0];
        system(cmd.c_str());
        system(chownCmd.c_str());
#endif
      }
      break;
    }

    case Action::Unmount:
    {
      if(Options::Name.empty())
      {
        printf("Missing <volumename>\n");
      }
      else
      {
        args.emplace_back(Options::Name);
        auto resp = SendReceive(args,bdcp::UNBIND);
        auto respParams = bdcp::Parse(resp);

        if(!((bdcp::BdResponse*)resp.get())->status)
        {
          printf("Mount path not found for volume '%s'.\n",Options::Name.c_str());
          exit(0);
        }
        printf("Unmounting complete.\n");
      }
      break;
    }

    case Action::Format:
    {
#if !defined(_WIN32)
      if(Options::Name.empty() || Options::Paths.size() != 1)
      {
        printf("Missing <volumename> or <fstype>\n");
      }
      else
      {
        args.emplace_back(Options::Name);
        auto resp = SendReceive(args,bdcp::BIND);
        auto respParams = bdcp::Parse(resp);

        if(!((bdcp::BdResponse*)resp.get())->status || respParams.size() != 1)
        {
          printf("Bind failed for volume '%s'.\n",Options::Name.c_str());
          exit(0);
        }

        int timeout = 10;	
        while(!nbd_ready(respParams[0].c_str()) && timeout--)
        {
          sleep(1);
        }

        if(timeout == 0)
        {
          printf("Bind timeout for volume '%s'.\n",Options::Name.c_str());
          exit(0);
        }

        printf("Formatting '%s' to '%s'...\n",respParams[0].c_str(),Options::Paths[0].c_str());
       
        std::string mkfs = "mkfs." + Options::Paths[0];
        
        std::string cmd = "sudo mkfs.";
        cmd += Options::Paths[0];
        for(auto arg : Options::ExternalArgs)
        {
          cmd += " " + arg;
        }
        cmd += " " + std::string(respParams[0]);
        system(cmd.c_str());
      }
#endif
      break;
    }

    case Action::Create:
    {
      if(Options::Name == "")
      {
        printf("Missing <volumeName>\n");
      }
      else 
      {
        printf("Creating volume '%s'...\n",Options::Name.c_str());
        VolumeManager::CreateVolume(Options::Name, Options::Size, Options::DataBlocks, Options::CodeBlocks);
      }
      break;
    }

    case Action::Delete:
    {
      if(Options::Name == "")
      {
        printf("Missing <volumeName>\n");
      }
      else 
      {
        bool success = VolumeManager::DeleteVolume(Options::Name, Options::Paths.size() > 0 ? Options::Paths.front() : "");
        if(success)
          printf("Volume '%s' deleted.\n",Options::Name.c_str());
      }
      break;
    }

    case Action::List:
    {
      ListVolumes();
      break;
    }

    default:
      printf("Unhandled option : %s\n",Action::ToString(Options::Action));
  }
}

int main(int argc, char * * argv)
{
  Options::Init(argc, argv);

  if(Options::KademliaUrl.size() == 0)
  {
    printf("Kademlia url's not found.\n");
    return 0;
  }

  VolumeManager::kademliaUrl = Options::KademliaUrl;

  HandleOptions();
  return 0;
}
