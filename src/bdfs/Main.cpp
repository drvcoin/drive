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
#include <unistd.h>
#include <string.h>
#include "Options.h"
#include "UnixDomainSocket.h"
#include "BdProtocol.h"

using namespace dfs;
using namespace bdfs;

bool SendReceive(const uint8_t *buff, uint32_t length)
{
  UnixDomainSocket socket;
  if(!socket.Connect("bdfsclient"))
  {
    printf("Error: Unable to connect to bdfsclient daemon.\n");
    return false;
  }

  if (socket.SendMessage(buff, length) != length)
  {
    printf("Error: Unable to sendmessage to bdfsclient daemon.\n");
    return false;
  }

  bdcp::BdResponse resp;
  if(socket.RecvMessage(&resp,sizeof(bdcp::BdResponse)) != resp.hdr.length)
  {
    printf("Error: Unable to readresponse from bdfsclient daemon.\n");
    return false;
  }

  return resp.success;
}

void HandleOptions()
{
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
        bdcp::BdMount mnt;
        mnt.hdr.length = sizeof(bdcp::BdMount);
        mnt.hdr.type = bdcp::Mount;
        strncpy(mnt.volumeName,Options::Name.c_str(),Options::Name.length());
        strncpy(mnt.path,Options::Paths[0].c_str(),Options::Paths[0].length());
        bool success = SendReceive((const uint8_t*)&mnt,mnt.hdr.length);
        printf("Mounting: %d\n",success);
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
        bdcp::BdCreate create;
        create.hdr.length = sizeof(bdcp::BdCreate);
        create.hdr.type = bdcp::Create;
        strncpy(create.volumeName, Options::Name.c_str(), Options::Name.length());
        strncpy(create.repoName, Options::Repo.c_str(), Options::Repo.length());
        create.dataBlocks = Options::DataBlocks;
        create.codeBlocks = Options::CodeBlocks;
        bool success = SendReceive((const uint8_t*)&create,create.hdr.length);
        if(success)
          printf("Config file : %s.config\n",Options::Name.c_str());
        else
          printf("Error creating volume.\n");
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
        bdcp::BdDelete del;
        del.hdr.length = sizeof(bdcp::BdDelete);
        del.hdr.type = bdcp::Delete;
        strncpy(del.volumeName, Options::Name.c_str(), Options::Name.length());
        bool success = SendReceive((const uint8_t*)&del,del.hdr.length);
        printf("Deleting volume: %s, success:%d\n",Options::Name.c_str(),success);
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
	HandleOptions();
	return 0;
}
