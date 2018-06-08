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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <assert.h>
#include "Global.h"
#include "HttpHandlerRegister.h"
#include "PartitionHandler.h"



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
#include "Global.h"


using namespace kad;



namespace bdhost
{
  static const char PATH[] = "/api/host/Kademlia";

  Kademlia * PartitionHandler::controller = nullptr;

  REGISTER_HTTP_HANDLER(Partition, PATH, new PartitionHandler());

  void PartitionHandler::ProcessRequest(bdhttp::HttpContext & context)
  {
    std::string path = context.path();
    if (path.size() <= sizeof(PATH) || path[sizeof(PATH) - 1] != '/')
    {
      if (path.size() == sizeof(PATH) && path == std::string(PATH) + '/')
      {
        context.writeResponse("{\"Name\":\"Partitions\",\"Path\":\"host://Partitions\",\"Type\":\"PartitionFolder\"}");
      }
      else
      {
        context.setResponseCode(404);
        context.writeError("Failed", "Object not found", bdhttp::ErrorCode::OBJECT_NOT_FOUND);
      }

      return;
    }

    auto pos = path.find('/', sizeof(PATH));
    if (pos == std::string::npos)
    {
      auto action = path.substr(sizeof(PATH));

      this->OnKademliaRequest(context, action);
    }
    else
    {
      printf("else\n");
    }
  }


  void PartitionHandler::OnKademliaRequest(bdhttp::HttpContext & context, const std::string & action)
  {
    if (action == "SetValue")
    {
      this->OnSetValue(context);
    }
    else if (action == "GetValue")
    {
      this->OnGetValue(context);
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Action not supported", bdhttp::ErrorCode::NOT_SUPPORTED);
    }
  }


  void PartitionHandler::OnSetValue(bdhttp::HttpContext & context)
  {
    printf("Kademlia::SetValue\n");

    std::string key(context.parameter("key"));
    std::string value(context.parameter("value"));

    uint64_t version = static_cast<uint64_t>(strtoull(context.parameter("version"), nullptr, 10));
    uint32_t ttl = static_cast<uint32_t>(strtoul(context.parameter("ttl"), nullptr, 10));


    printf("%s %s %lu %u\n",key.c_str(), value.c_str(), version, ttl);

    sha1_t digest;
    Digest::Compute(key.c_str(), key.size(), digest);

    KeyPtr keyptr = std::make_shared<Key>(digest);


    uint8_t * buffer = nullptr;

    size_t size = value.size();

    buffer = new uint8_t[size];

    memcpy(buffer, value.c_str(), size);


    auto data = std::make_shared<Buffer>(buffer, size, false, true);
    
    auto result = AsyncResultPtr(new AsyncResult<bool>());
    
    PartitionHandler::controller->Store(keyptr, data, ttl, version, result);

    
    result->Wait();
    
    if (!AsyncResultHelper::GetResult<bool>(result.get()))
    {
      printf("ERROR: failed to set value.\n");
      context.setResponseCode(500);
      context.writeError("Failed", "Failed to set value", bdhttp::ErrorCode::GENERIC_ERROR);
    } 

    context.writeResponse("true");
  }

  void PartitionHandler::OnGetValue(bdhttp::HttpContext & context)
  {
    printf("Kademlia::GetValue\n");

    std::string key(context.parameter("key"));

    sha1_t digest;
    Digest::Compute(key.c_str(), key.size(), digest);

    KeyPtr keyptr = std::make_shared<Key>(digest);

    auto result = AsyncResultPtr(new  AsyncResult<BufferPtr>());

    PartitionHandler::controller->FindValue(keyptr, result);

    result->Wait();

    auto buffer = AsyncResultHelper::GetResult<BufferPtr>(result.get());

    if (buffer && buffer->Size() > 0)
    {
      printf("%s\n", std::string(reinterpret_cast<const char *>(buffer->Data()), buffer->Size()).c_str());
      context.writeResponse(buffer->Data(), buffer->Size());
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Value not found", bdhttp::ErrorCode::GENERIC_ERROR);
    }
  }

}

