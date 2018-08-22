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
#include "devio.h"

namespace dfs
{
  using namespace bdfs;

  ClientManager::ClientManager()
  {
  }

  bool ClientManager::Start()
  {
    Listen();
    return true;
  }

  bool ClientManager::Stop()
  {
    return true;
  }

  std::unique_ptr<char[]> ClientManager::ProcessRequest(std::unique_ptr<char[]> &buff, bool &shouldReply, PiperIPC &ipc)
  {
    shouldReply = true;

    uint8_t status = 0;
    std::vector<std::string> args;

    auto inArgs = bdcp::Parse(buff);
    printf("ProcessRequest::instruction of type : %d\n", ((bdcp::BdHdr *)buff.get())->type);
    switch (((bdcp::BdHdr *)buff.get())->type)
    {
    case bdcp::BIND:
    {
      if (inArgs.size() > 0)
      {
        status = ActionHandler::BindVolume(inArgs[0], inArgs.size() > 1 ? inArgs[1] : "");
        if (status)
        {
          std::string shm = drv_imdisk_shm_name(inArgs[0]);
          args.emplace_back(shm);
        }
      }
      break;
    }

    case bdcp::UNBIND:
    {
      if (inArgs.size() == 1)
      {
        std::string mountPath = ActionHandler::GetMountPathForVolume(inArgs[0]);
        status = ActionHandler::UnbindVolume(inArgs[0]);
        if (status)
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

      for (auto &it : ActionHandler::GetVolumeInfo())
      {
        printf("QUERY_%s,%s\n", it.first.c_str(), it.second.c_str());
        args.emplace_back(it.first);
        args.emplace_back(it.second);

        auto p = bdcp::Create(bdcp::RESPONSE, args, status);

        ipc.SyncWrite((uint8_t*)p.get(), ((bdcp::BdHdr*)p.get())->length);
        args.clear();
      }
      break;
    }

    default:
      printf("Unhandled instruction of type : %d\n", ((bdcp::BdHdr *)buff.get())->type);
    }

    return bdcp::Create(bdcp::RESPONSE, args, status);
  }

  void ClientManager::Listen()
  {
    bool fContinue = true;

    PiperIPC ipc;

    while (fContinue)
    {
      bool fSuccess;


      if (!ipc.CreateEndPoint(1))
      {
        printf("IPC Exception: GLE:%u\n", GetLastError());
        ipc.Disconnect();
        continue;
      }

      DWORD cbLen = 0;
      uint32_t length = 0;
      ipc.SyncRead((uint8_t*)&length, sizeof(uint32_t), cbLen);


      if (cbLen != sizeof(uint32_t))
      {
        printf("%s: failed to read message length.\n", __func__);
      }

      std::unique_ptr<char[]>buff = std::make_unique<char[]>(length);
      ((bdcp::BdHdr*)buff.get())->length = length;

      ipc.SyncRead((uint8_t*)buff.get() + sizeof(uint32_t), length - sizeof(uint32_t), cbLen);
      if (cbLen == length - sizeof(uint32_t))
      {
        bool shouldReply = true;
        auto resp = this->ProcessRequest(buff, shouldReply, ipc);

        if (shouldReply)
        {
          ipc.SyncWrite((uint8_t*)resp.get(), ((bdcp::BdHdr*)resp.get())->length);
        }
      }

      ipc.Disconnect();
    }
  }
}
