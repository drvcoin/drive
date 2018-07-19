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
#include <json/json.h>
#include "HttpHandlerRegister.h"
#include "KademliaHandler.h"



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
#include "TcpTransport.h"
#include "Digest.h"
#include "Kademlia.h"

#include <arpa/inet.h>

#include "Options.h"


using namespace kad;



namespace bdhost
{
  static const char PATH[] = "/api/host/Query";

  Kademlia * KademliaHandler::controller = nullptr;

  REGISTER_HTTP_HANDLER(Kademlia, PATH, new KademliaHandler());

  void KademliaHandler::ProcessRequest(bdhttp::HttpContext & context)
  {
    std::string path = context.path();
    if (path.size() <= sizeof(PATH) || path[sizeof(PATH) - 1] != '/')
    {
      if (path.size() == sizeof(PATH) && path == std::string(PATH) + '/')
      {
        context.writeResponse("{\"Name\":\"Kademlia\",\"Path\":\"host://Kademlia\",\"Type\":\"Kademlia\"}");
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
      context.setResponseCode(404);
      context.writeError("Failed", "Object not found", bdhttp::ErrorCode::OBJECT_NOT_FOUND);
    }
  }


  void KademliaHandler::OnKademliaRequest(bdhttp::HttpContext & context, const std::string & action)
  {
    if (action == "Publish")
    {
      this->Publish(context);
    }
    else if (action == "Query")
    {
      this->Query(context);
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Action not supported", bdhttp::ErrorCode::NOT_SUPPORTED);
    }
  }


  void KademliaHandler::Publish(bdhttp::HttpContext & context)
  {
    printf("Kademlia::SetValue\n");

    std::string value(context.parameter("value"));

    Json::Value json;
    Json::Reader reader;
    if (reader.parse(value.c_str(), value.size(), json, false) && json.isString())
    {
      value = json.asString();
    }

    uint8_t * buffer = nullptr;
    size_t size = value.size();
    buffer = new uint8_t[size];
    memcpy(buffer, value.c_str(), size);


    char * d = (char*)buffer;
    printf("value: %s\n", d);
    sha1_t dig;
    Digest::Compute(d, size, dig);
    Digest::Print(dig);
    KeyPtr key = std::make_shared<Key>(dig);


    auto data = std::make_shared<Buffer>(buffer, size, false, true);

    auto result = AsyncResultPtr(new AsyncResult<bool>());

    KademliaHandler::controller->Publish(key, data, 999, 1, result);

    if (!result->Wait(60000) || !AsyncResultHelper::GetResult<bool>(result.get()))
    {
      printf("ERROR: failed to set value.\n");
      context.setResponseCode(500);
      context.writeError("Failed", "Failed to set value", bdhttp::ErrorCode::GENERIC_ERROR);
    }

    context.writeResponse("true");
  }

  void KademliaHandler::Query(bdhttp::HttpContext & context)
  {
    printf("Kademlia::GetValue\n");

    std::string query(context.parameter("query"));

    sha1_t digest;
    Digest::Compute(query.c_str(), query.size(), digest);
    KeyPtr key = std::make_shared<Key>(digest);

    printf("query: %s\n", query.c_str());

    auto result = AsyncResultPtr(new  AsyncResult<BufferPtr>());

    KademliaHandler::controller->Query(key, query, result);

    if (!result->Wait(60000))
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Internal Error", bdhttp::ErrorCode::GENERIC_ERROR);
      return;
    }

    auto buffer = AsyncResultHelper::GetResult<BufferPtr>(result.get());

    if (buffer && buffer->Size() > 0)
    {
      printf("RESULT: %s\n", std::string(reinterpret_cast<const char *>(buffer->Data()), buffer->Size()).c_str());
      Json::Value json = std::string(static_cast<const char *>(buffer->Data()), buffer->Size());
      auto resp = json.toStyledString();
      context.writeResponse(resp.c_str(), resp.size());
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Value not found", bdhttp::ErrorCode::GENERIC_ERROR);
    }
  }

}

