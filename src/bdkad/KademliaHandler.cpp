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

#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <json/json.h>

#include "HttpHandlerRegister.h"
#include "KademliaHandler.h"
#include "Digest.h"


using namespace kad;


namespace bdhost
{
  static const char PATH[] = "/api/host/Kademlia";

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
    if (action == "SetValue")
    {
      this->OnSetValue(context);
    }
    else if (action == "GetValue")
    {
      this->OnGetValue(context);
    }
    else if (action == "Publish")
    {
      this->OnPublish(context);
    }
    else if (action == "Query")
    {
      this->OnQuery(context);
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Action not supported", bdhttp::ErrorCode::NOT_SUPPORTED);
    }
  }


  void KademliaHandler::OnSetValue(bdhttp::HttpContext & context)
  {
    printf("Kademlia::OnSetValue\n");

    std::string key(context.parameter("key"));
    std::string value(context.parameter("value"));

    Json::Value json;
    Json::Reader reader;
    if (reader.parse(key.c_str(), key.size(), json, false) && json.isString())
    {
      key = json.asString();
    }

    if (reader.parse(value.c_str(), value.size(), json, false) && json.isString())
    {
      value = json.asString();
    }

    uint64_t version = static_cast<uint64_t>(strtoull(context.parameter("version"), nullptr, 10));
    uint32_t ttl = static_cast<uint32_t>(strtoul(context.parameter("ttl"), nullptr, 10));

    if (key.empty())
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Paramter 'key' missing or invalid", bdhttp::ErrorCode::ARGUMENT_INVALID);
      return;
    }

    if (value.empty())
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Paramter 'value' missing or invalid", bdhttp::ErrorCode::ARGUMENT_INVALID);
      return;
    }


    sha1_t digest;
    Digest::Compute(key.c_str(), key.size(), digest);

    KeyPtr keyptr = std::make_shared<Key>(digest);

    uint8_t * buffer = nullptr;

    size_t size = value.size();

    buffer = new uint8_t[size];

    memcpy(buffer, value.c_str(), size);

    auto data = std::make_shared<Buffer>(buffer, size, false, true);

    auto result = AsyncResultPtr(new AsyncResult<bool>());

    KademliaHandler::controller->Store(keyptr, data, ttl, version, result);

    if (!result->Wait(60000) || !AsyncResultHelper::GetResult<bool>(result.get()))
    {
      printf("ERROR: failed to set value.\n");
      context.setResponseCode(500);
      context.writeError("Failed", "Failed to set value", bdhttp::ErrorCode::GENERIC_ERROR);
    }

    context.writeResponse("true");
  }

  void KademliaHandler::OnGetValue(bdhttp::HttpContext & context)
  {
    printf("Kademlia::OnGetValue\n");

    std::string key(context.parameter("key"));

    Json::Value json;
    Json::Reader reader;
    if (reader.parse(key.c_str(), key.size(), json, false) && json.isString())
    {
      key = json.asString();
    }

    if (key.empty())
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Paramter 'key' missing or invalid", bdhttp::ErrorCode::ARGUMENT_INVALID);
      return;
    }


    sha1_t digest;
    Digest::Compute(key.c_str(), key.size(), digest);

    KeyPtr keyptr = std::make_shared<Key>(digest);

    auto result = AsyncResultPtr(new AsyncResult<BufferPtr>());

    KademliaHandler::controller->FindValue(keyptr, result);

    if (!result->Wait(60000))
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Internal Error", bdhttp::ErrorCode::GENERIC_ERROR);
      return;
    }

    auto buffer = AsyncResultHelper::GetResult<BufferPtr>(result.get());

    if (buffer && buffer->Size() > 0)
    {
      printf("%s\n", std::string(reinterpret_cast<const char *>(buffer->Data()), buffer->Size()).c_str());
      json = std::string(static_cast<const char *>(buffer->Data()), buffer->Size());
      auto resp = json.toStyledString();
      context.writeResponse(resp.c_str(), resp.size());
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Value not found", bdhttp::ErrorCode::GENERIC_ERROR);
    }
  }

  void KademliaHandler::OnPublish(bdhttp::HttpContext & context)
  {
    printf("Kademlia::OnPublish\n");

    std::string value(context.parameter("value"));

    Json::Value json;
    Json::Reader reader;
    if (reader.parse(value.c_str(), value.size(), json, false) && json.isString())
    {
      value = json.asString();
    }

    if (value.empty())
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Paramter 'value' missing or invalid", bdhttp::ErrorCode::ARGUMENT_INVALID);
      return;
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

  void KademliaHandler::OnQuery(bdhttp::HttpContext & context)
  {
    printf("Kademlia::OnQuery\n");

    std::string query(context.parameter("query"));
    uint32_t limit = static_cast<uint32_t>(strtoul(context.parameter("limit"), nullptr, 10));

    Json::Value json;
    Json::Reader reader;
    if (reader.parse(query.c_str(), query.size(), json, false) && json.isString())
    {
      query = json.asString();
    }

    if (query.empty())
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Paramter 'query' missing or invalid", bdhttp::ErrorCode::ARGUMENT_INVALID);
      return;
    }

    if (!limit)
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Paramter 'limit' missing or invalid", bdhttp::ErrorCode::ARGUMENT_INVALID);
      return;
    }

    sha1_t digest;
    Digest::Compute(query.c_str(), query.size(), digest);
    KeyPtr key = std::make_shared<Key>(digest);

    auto result = AsyncResultPtr(new  AsyncResult<BufferPtr>());

    KademliaHandler::controller->Query(key, query, limit, result);

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
      context.writeResponse(static_cast<const char *>(buffer->Data()), buffer->Size());
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Value not found", bdhttp::ErrorCode::GENERIC_ERROR);
    }
  }
}
