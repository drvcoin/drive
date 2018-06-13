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

namespace dfs
{
  using namespace bdfs;

  ClientManager::ClientManager() :
    isUnixSocketListening(false),
    requestLoop(&ClientManager::HandleRequest)
  {
    VolumeManager::defaultConfig.ConnectTimeout(5);
    VolumeManager::defaultConfig.RequestTimeout(5);
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

    std::thread th(&ClientManager::Listen, this);
    th.detach();
  
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

  std::unique_ptr<bdcp::BdResponse> ClientManager::ProcessRequest(const bdcp::BdHdr *pHdr, UnixDomainSocket *socket, bool &shouldReply)
  {
    shouldReply = true;

    std::unique_ptr<bdcp::BdResponse> resp = std::make_unique<bdcp::BdResponse>();;
    resp->hdr.length = sizeof(bdcp::BdResponse);
    resp->hdr.type = bdcp::RESPONSE;
    resp->status = 0;
    
    bdcp::BdRequest *pReq = (bdcp::BdRequest *)pHdr;

    switch(pHdr->type)
    {
      case bdcp::BIND:
      {
        resp->status = ActionHandler::BindVolume(pReq->data1,pReq->data2);
        if(resp->status)
        {
          std::string nbdPath = ActionHandler::GetNbdForVolume(pReq->data1);
          strncpy(resp->data1,nbdPath.c_str(), std::min(nbdPath.length(),sizeof(resp->data1)));
        }
        break;
      }

      case bdcp::UNBIND:
      {
        std::string mountPath = ActionHandler::GetMountPathForVolume(std::string(pReq->data1));
        resp->status = ActionHandler::UnbindVolume(pReq->data1);
        if(resp->status)
        {
          strncpy(resp->data1,mountPath.c_str(), std::min(mountPath.length(),sizeof(resp->data1)));
        }
        break;
      }

      case bdcp::QUERY_VOLUMEINFO:
      {
        shouldReply = false;
        resp->status = 1;
        
        for(auto &it : ActionHandler::GetVolumeInfo())
        {
          printf("QUERY_%s,%s,%s\n",it.second->volumeName.c_str(),it.second->nbdPath.c_str(),it.second->mountPath.c_str());
          strncpy(resp->data1,it.second->volumeName.c_str(), std::min(it.second->volumeName.length(),sizeof(resp->data1)));
          strncpy(resp->data2,it.second->nbdPath.c_str(), std::min(it.second->nbdPath.length(),sizeof(resp->data2)));
          strncpy(resp->data3,it.second->mountPath.c_str(), std::min(it.second->mountPath.length(),sizeof(resp->data3)));
          socket->SendMessage(resp.get(),resp->hdr.length);
        }
        break;
      }

      default:
        printf("Unhandled instruction of type : %d\n",pHdr->type);
    }
   
    return resp;
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

    uint8_t *buff = new uint8_t[length];
    bdcp::BdHdr *pHdr = (bdcp::BdHdr *)(buff);
    pHdr->length = length;

		if (socket->RecvMessage(buff+sizeof(uint32_t), length-sizeof(uint32_t)) > 0)
    {
      bool shouldReply = true;
      auto resp = _this->ProcessRequest((bdcp::BdHdr *)buff,socket,shouldReply);
      
      if(shouldReply)
      {
        socket->SendMessage(resp.get(),resp->hdr.length);
      }
      resp.reset();
    }
    else
    {
      printf("%s: failed to read bdfs message.\n",__func__);
    }
  
    delete(buff);
    socket->Close();
    
    return true;
	}

	void ClientManager::Listen()
	{
    isUnixSocketListening = true;
		for(;;)
		{
			UnixDomainSocket * socket = this->unixSocket.Accept();

			if (socket == NULL)
			{
				break;
			}

			this->requestLoop.SendEvent(this, socket);
		}
	}
}
