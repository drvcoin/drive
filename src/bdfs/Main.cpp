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

#include "UnixDomainSocket.h"
#include "BdProtocol.h"

using namespace dfs;
using namespace bdfs;

static bdfs::HttpConfig defaultConfig;

#define SENDRECV_TIMEOUT 30

bdcp::BdResponse SendReceive(const uint8_t *buff, uint32_t length)
{
  UnixDomainSocket socket;
  bdcp::BdResponse resp;
  resp.status = false;

  if(!socket.Connect("bdfsclient"))
  {
    sprintf(&resp.data[0],"Error: Unable to connect to bdfsclient daemon.\n");
  }
  else if (socket.SendMessage(buff, length,SENDRECV_TIMEOUT) != length)
  {
    sprintf(&resp.data[0],"Error: Unable to sendmessage to bdfsclient daemon.\n");
  }
  else if(socket.RecvMessage(&resp,sizeof(bdcp::BdResponse),SENDRECV_TIMEOUT) != resp.hdr.length)
  {
    sprintf(&resp.data[0],"Error: Unable to readresponse from bdfsclient daemon.\n");
  }

  return resp;
}

void HandleOptions()
{
  bdcp::BdRequest request;
  memset(&request,0,sizeof(bdcp::BdRequest));
  request.hdr.length = sizeof(bdcp::BdRequest);

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
        request.hdr.type = bdcp::BIND;
        strncpy(request.data,Options::Name.c_str(),Options::Name.length());
        bdcp::BdResponse resp = SendReceive((const uint8_t*)&request,request.hdr.length);

        if(!resp.status)
        {
          printf("Bind failed for volume '%s'.\n",Options::Name.c_str());
          exit(0);
        }

			  int timeout = 10;	
				while(!nbd_ready(&resp.data[0]) && timeout--)
        {
          sleep(1);
        }

        if(timeout == 0)
        {
          printf("Bind timeout for volume '%s'.\n",Options::Name.c_str());
          exit(0);
        }

        printf("Mounting '%s' to '%s'\n",resp.data,Options::Paths[0].c_str());
        
        char *args[] = {"sudo", "mount", resp.data, (char*)Options::Paths[0].c_str(), NULL};
        
        execvp("sudo",args);
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
        request.hdr.type = bdcp::QUERY_NBD;
        strncpy(request.data,Options::Name.c_str(),Options::Name.length());
        bdcp::BdResponse resp = SendReceive((const uint8_t*)&request,request.hdr.length);

        if(!resp.status)
        {
          printf("Bind failed for volume '%s'.\n",Options::Name.c_str());
          exit(0);
        }

        printf("Unmounting '%s'...\n",resp.data);
        
        char *args[] = {"sudo", "umount", resp.data, NULL};
        
        execvp("sudo",args);
      }
      break;
    }

    case Action::Format:
    {
      if(Options::Name.empty() || Options::Paths.size() != 1)
      {
        printf("Missing <volumename> or <fstype>\n");
      }
      else
      {
        request.hdr.type = bdcp::BIND;
        strncpy(request.data,Options::Name.c_str(),Options::Name.length());
        bdcp::BdResponse resp = SendReceive((const uint8_t*)&request,request.hdr.length);

        if(!resp.status)
        {
          printf("Bind failed for volume '%s'.\n",Options::Name.c_str());
          exit(0);
        }

			  int timeout = 10;	
				while(!nbd_ready(&resp.data[0]) && timeout--)
        {
          sleep(1);
        }

        if(timeout == 0)
        {
          printf("Bind timeout for volume '%s'.\n",Options::Name.c_str());
          exit(0);
        }

        printf("Formatting '%s' to '%s'...\n",resp.data,Options::Paths[0].c_str());
       
        std::string mkfs = "mkfs." + Options::Paths[0];
        char *args[] = {"sudo", (char*)mkfs.c_str(), resp.data, NULL};
        
        execvp("sudo",args);
      }
      break;
    }

    case Action::Create:
    {
      if(Options::Name == "" || Options::Repo == "")
      {
        printf("Missing <volumeName> or <contractRepo>\n");
      }
      else 
      {
        printf("Creating volume '%s'...\n",Options::Name.c_str());
        bool success = VolumeManager::CreateVolume(Options::Name, Options::Repo, Options::DataBlocks, Options::CodeBlocks);
        if(success)
          printf("Config file : %s.config\n",Options::Name.c_str());
        else
          printf("Error creating volume '%s'.\n", Options::Name.c_str());
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
        bool success = VolumeManager::DeleteVolume(Options::Name);
        if(success)
          printf("Volume '%s' deleted.\n",Options::Name.c_str());
      }
      break;
    }
  
    

    default:
      printf("Unhandled option : %s\n",Action::ToString(Options::Action));
  }
}

int main(int argc, char * * argv)
{
  Options::Init(argc, argv);

  VolumeManager::defaultConfig.ConnectTimeout(5);
  VolumeManager::defaultConfig.RequestTimeout(5);

  VolumeManager::kademliaUrl = Options::KademliaUrl;

  HandleOptions();
  return 0;
}
