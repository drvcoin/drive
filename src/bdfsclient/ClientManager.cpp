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

namespace dfs
{
  using namespace bdfs;

  ClientManager::ClientManager() :
    isUnixSocketListening(false),
    requestLoop(&ClientManager::HandleRequest)
  {
    ActionHandler::defaultConfig.ConnectTimeout(5);
    ActionHandler::defaultConfig.RequestTimeout(5);
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
		isUnixSocketListening = false;
		unixSocket.Shutdown(SHUT_RDWR);
		unixSocket.Close();

		this->requestLoop.Stop();

		return true;
	}

  bool ClientManager::ProcessRequest(const bdcp::BdHdr *pHdr)
  {
    bool success = true;
    switch(pHdr->type)
    {
      case bdcp::Mount:
      {
        bdcp::BdMount *pMnt = (bdcp::BdMount *)pHdr;
        success = ActionHandler::MountVolume(pMnt->volumeName, pMnt->path);
        break;
      }

      case bdcp::Unmount:
      {
        break;
      }

      case bdcp::Create:
      {
        bdcp::BdCreate *pCreate = (bdcp::BdCreate *)pHdr;
        success = ActionHandler::CreateVolume(pCreate->volumeName, pCreate->repoName, pCreate->dataBlocks, pCreate->codeBlocks);
        break;
      }

      case bdcp::Delete:
      {
        bdcp::BdDelete *pDelete = (bdcp::BdDelete *)pHdr;
        success = ActionHandler::DeleteVolume(pDelete->volumeName);
        break;
      }

      default:
        printf("Unhandled instruction of type : %d\n",pHdr->type);
    }
    return success;
  }

	bool ClientManager::HandleRequest(void * sender, UnixDomainSocket * socket)
	{
		ClientManager *_this = (ClientManager *)sender;

    uint32_t length;

    bdcp::BdResponse resp;
    resp.hdr.length = sizeof(bdcp::BdResponse);
    resp.hdr.type = bdcp::Response;
    resp.success = 0;
	 
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
      resp.success = _this->ProcessRequest((bdcp::BdHdr *)buff);
    }
    else
    {
      printf("%s: failed to read bdfs message.\n",__func__);
    }
  
    delete(buff);
    socket->SendMessage(&resp,resp.hdr.length);
    //socket->Close();
    
    return true;
	}

	void ClientManager::Listen()
	{
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
