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

#include "ClientManager.h"
#include <signal.h>
#include <stdio.h>
#include <memory>
#include <thread>
#include "ActionHandler.h"
#include "VolumeManager.h"
#include "BdProtocol.h"

namespace dfs
{
  using namespace bdfs;

  ClientManager::ClientManager() :
    isUnixSocketListening(false),
    requestLoop(&ClientManager::HandleRequest)
  {
  }

  bool ClientManager::Start()
  {
    (void) signal(SIGPIPE, SIG_IGN);

    if (!unixSocket.Listen("bdfsclient", 2))
    {
      printf("Failed to listen on Unix socket\n");
      return false;
    }

    if (!this->requestLoop.Start())
    {
      printf("Failed to start request loop\n");
      return false;
    }

    // Listen loop
    std::thread([&]{
      isUnixSocketListening = true;
      while(true)
      {
        UnixDomainSocket * socket = this->unixSocket.Accept();

        if (socket == NULL)
        {
          break;
        }

        this->requestLoop.SendEvent(this, socket);
      }
    }).join();
    return true;
  }

  bool ClientManager::Stop()
  {
    if(isUnixSocketListening)
    {
      unixSocket.Shutdown(SHUT_RDWR);
      unixSocket.Close();
    }
    
    isUnixSocketListening = false;
    this->requestLoop.Stop();

    return true;
  }

  std::unique_ptr<char[]> ClientManager::ProcessRequest(std::unique_ptr<char[]> &buff, UnixDomainSocket *socket, bool &shouldReply)
  {
    shouldReply = true;

    uint8_t status = 0;
    std::vector<std::string> args;

    auto inArgs = bdcp::Parse(buff);
    
    switch(((bdcp::BdHdr *)buff.get())->type)
    {
      case bdcp::BIND:
      {
        if(inArgs.size() > 0)
        {
          status = ActionHandler::BindVolume(inArgs[0], inArgs.size() > 1 ? inArgs[1]: "");
          if(status)
          {
            std::string nbdPath = ActionHandler::GetNbdForVolume(inArgs[0]);
            args.emplace_back(nbdPath);
          }
        }
        break;
      }

      case bdcp::UNBIND:
      {
        if(inArgs.size() == 1)
        {
          std::string mountPath = ActionHandler::GetMountPathForVolume(inArgs[0]);
          status = ActionHandler::UnbindVolume(inArgs[0]);
          if(status)
          {
            args.emplace_back(mountPath);
          }
        }
        break;
      }

      case bdcp::QUERY_VOLUMEINFO:
      {
        shouldReply = false;
        status = 1;
        
        for(auto &it : ActionHandler::GetVolumeInfo())
        {
          printf("QUERY_%s,%s,%s\n",it.second->volumeName.c_str(),it.second->nbdPath.c_str(),it.second->mountPath.c_str());
          args.emplace_back(it.second->volumeName);
          args.emplace_back(it.second->nbdPath);
          args.emplace_back(it.second->mountPath);

          auto p = bdcp::Create(bdcp::RESPONSE,args,status);
          
          socket->SendMessage(p.get(),((bdcp::BdHdr *)p.get())->length);
        }
        break;
      }

      default:
        printf("Unhandled instruction of type : %d\n",((bdcp::BdHdr *)buff.get())->type);
    }
   
    return bdcp::Create(bdcp::RESPONSE,args,status);
  }

  bool ClientManager::HandleRequest(void * sender, UnixDomainSocket * socket)
  {
    ClientManager *_this = (ClientManager *)sender;

    uint32_t length;
   
    if (socket->RecvMessage(&length, sizeof(uint32_t)) <= 0)
    {
      printf("%s: failed to read message length.\n", __func__);
      socket->Close();
      return false;
    }

    std::unique_ptr<char[]>buff = std::make_unique<char[]>(length);
    ((bdcp::BdHdr*)buff.get())->length = length;

    if (socket->RecvMessage(buff.get()+sizeof(uint32_t), length-sizeof(uint32_t)) > 0)
    {
      bool shouldReply = true;
      auto resp = _this->ProcessRequest(buff,socket,shouldReply);
      
      if(shouldReply)
      {
        socket->SendMessage(resp.get(),((bdcp::BdHdr*)resp.get())->length);
      }
    }
    else
    {
      printf("%s: failed to read bdfs message.\n",__func__);
    }
 
    socket->Close();
    
    return true;
  }
}
