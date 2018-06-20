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

#include "KademliaModule.h"

#include <memory.h>
#include <sys/stat.h>



#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <iterator>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "Contact.h"
#include "Config.h"
#include "TransportFactory.h"
#include "LinuxFileTransport.h"
#include "TcpTransport.h"
#include "Digest.h"
#include "Kademlia.h"

#include <arpa/inet.h>


#include "Options.h"


#include "KademliaHandler.h"


using namespace kad;



namespace bdhost
{

  KademliaModule::KademliaModule()
  {
  }

  KademliaModule::~KademliaModule()
  {
  }

class TransportFactoryImpl : public TransportFactory
{
public:

  std::unique_ptr<ITransport> Create() override
  {
    return std::unique_ptr<ITransport>(new TcpTransport());
  }
};

static void SetVerbose(const std::string & option)
{
  if (option == "on")
  {
    Config::SetVerbose(true);
  }
  else if (option == "off")
  {
    Config::SetVerbose(false);
  }
  else
  {
    printf("ERROR: Unknown verbose option.\n");
  }
}


static void InitKey(const char * rootPath)
{ 
  sha1_t digest;
  Digest::Compute(rootPath, strlen(rootPath), digest);
  
  KeyPtr key = std::make_shared<Key>(digest);
  
  FILE * file = fopen((std::string(rootPath) + "/key").c_str(), "w");
  
  if (file)
  { 
    fwrite(key->Buffer(), 1, Key::KEY_LEN, file);
    fclose(file);
  }

}

  void KademliaModule::Initialize()
  {
    printf("[KAD] Initialize\n");

    SetVerbose("on");

    printf("root path: %s\n", bdhost::Options::k_root);

    printf("kademlia node: addr = %08X port=%u\n", bdhost::Options::k_addr, bdhost::Options::k_port);

    Contact self;
    self.addr = bdhost::Options::k_addr;
    self.port = bdhost::Options::k_port;

    mkdir(bdhost::Options::k_root, 0755);
    InitKey(bdhost::Options::k_root);

    Config::Initialize(bdhost::Options::k_root);

    Key selfKey{Config::NodeId()->Buffer()};

    Config::Initialize(selfKey, self);

    TransportFactory::Reset(new TransportFactoryImpl());


    KademliaHandler::controller = new Kademlia();
    KademliaHandler::controller->Initialize();

    printf("[KAD] Ready?\n");

    while(!KademliaHandler::controller->IsReady())
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    printf("[KAD] Ready!\n");

  }
}
