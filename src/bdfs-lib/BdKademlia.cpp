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

#include <assert.h>
#include <stdio.h>
#include <json/json.h>
#include "BdTypes.h"
#include "Base64Encoder.h"
#include "BdKademlia.h"

namespace bdfs
{
  BD_TYPE_REG(Kademlia, BdKademlia);


  BdKademlia::BdKademlia(const char * base, const char * name, const char * path, const char * type)
    : BdObject(base, name, path, type)
  {
  }


  AsyncResultPtr<bool> BdKademlia::SetValue(const char * key, const void * value, size_t size, uint64_t version, uint32_t ttl)
  {
    assert(key);
    assert(value);

    size_t encodedLen = Base64Encoder::GetEncodedLength(size);
    
    std::string encoded;
    encoded.resize(encodedLen);

    encodedLen = Base64Encoder::Encode(static_cast<const uint8_t *>(value), size, &encoded[0], encodedLen);
    if (encodedLen != encoded.size())
    {
      encoded.resize(encodedLen);
    }

    BdObject::CArgs args;
    args["key"] = std::string(key);
    args["value"] = encoded;
    args["version"] = Json::Value::UInt(version);
    args["ttl"] = Json::Value::UInt(ttl);

    auto result = std::make_shared<AsyncResult<bool>>();

    bool rtn = this->Call("SetValue", args,
      [result](Json::Value & response, bool error)
      {
        result->SetError(error);
        if (error || !response.isBool())
        {
          result->Complete(false);
        }
        else
        {
          result->Complete(response.asBool());
        }
      }
    );

    return rtn ? result : nullptr;
  }


  AsyncResultPtr<Buffer> BdKademlia::GetValue(const char * key)
  {
    assert(key);

    BdObject::CArgs args;
    args["key"] = std::string(key);

    auto result = std::make_shared<AsyncResult<Buffer>>();

    bool rtn = this->Call("GetValue", args,
      [result](Json::Value & response, bool error)
      {
        result->SetError(error);
        if (error || !response.isString())
        {
          result->Complete(Buffer());
        }
        else
        {
          auto str = response.asString();

          size_t decodedLen = Base64Encoder::GetDecodedLength(str.size());

          Buffer buffer;
          buffer.Resize(decodedLen);

          decodedLen = Base64Encoder::Decode(str.c_str(), str.size(), static_cast<uint8_t *>(buffer.Buf()), decodedLen);
          if (decodedLen < buffer.Size())
          {
            buffer.Resize(decodedLen);
          }

          result->Complete(std::move(buffer));
        }
      }
    );

    return rtn ? result : nullptr;
  }

  AsyncResultPtr<bool> BdKademlia::PublishStorage(const char * node, const char * contract, const size_t storage, const size_t reputation = 1)
  {
    assert(node);

    Json::Value value;
    value["type"] = "storage";
    value["name"] = std::string(node);
    value["contract"] = std::string(contract);
    value["size"] = Json::Value::UInt(storage);
    value["reputation"] = Json::Value::UInt(reputation);

    BdObject::CArgs args;
    args["value"] = value.toStyledString();;

    auto result = std::make_shared<AsyncResult<bool>>();

    bool rtn = this->Call("Publish", args,
      [result](Json::Value & response, bool error)
      {
        result->SetError(error);
        if (error || !response.isBool())
        {
          result->Complete(false);
        }
        else
        {
          result->Complete(response.asBool());
        }
      }
    );

    return rtn ? result : nullptr;
  }


  AsyncResultPtr<Json::Value> BdKademlia::QueryStorage(const char * query, size_t limit)
  {
    assert(query);

    BdObject::CArgs args;
    args["query"] = std::string(query);
    args["limit"] = Json::Value::UInt(limit);

    auto result = std::make_shared<AsyncResult<Json::Value>>();
    bool rtn = this->Call("Query", args,
      [result](Json::Value & response, bool error)
      {
        result->SetError(error);
        if (error || !response.isArray())
        {
          result->Complete(Json::Value());
        }
        else
        {
          result->Complete(std::move(response));
        }
      }
    );

    return rtn ? result : nullptr;
  }


  AsyncResultPtr<bool> BdKademlia::PublishRelay(const RelayInfo & reg)
  {
    Json::Value value;
    value["type"] = "relay";
    value["name"] = reg.name;

    for (const auto & endpoint : reg.endpoints)
    {
      Json::Value ep;
      ep["host"] = endpoint.host;
      ep["socks"] = Json::Value::UInt(endpoint.socksPort);
      ep["quic"] = Json::Value::UInt(endpoint.quicPort);

      value["endpoints"].append(ep);
    }

    BdObject::CArgs args;
    args["value"] = value.toStyledString();

    auto result = std::make_shared<AsyncResult<bool>>();

    bool rtn = this->Call("Publish", args,
      [result](Json::Value & response, bool error)
      {
        result->SetError(error);
        if (error || !response.isBool())
        {
          result->Complete(false);
        }
        else
        {
          result->Complete(response.asBool());
        }
      }
    );

    return rtn ? result : nullptr;
  }


  static bool parseRelayInfo(Json::Value & obj, RelayInfo & output)
  {
    if (!obj.isObject() || obj.isNull())
    {
      return false;
    }

    if (!obj["type"].isString() || obj["type"].asString() != "relay")
    {
      return false;
    }

    if (obj["name"].isString())
    {
      output.name = obj["name"].asString();
    }

    auto & endpoints = obj["endpoints"];
    if (endpoints.isArray() && !endpoints.isNull())
    {
      for (size_t i = 0; i < endpoints.size(); ++i)
      {
        auto & ep = endpoints[i];
        if (!ep.isObject() || ep.isNull())
        {
          continue;
        }

        if (!ep["host"].isString() || !ep["socks"].isIntegral() || !ep["quic"].isIntegral())
        {
          continue;
        }

        RelayInfo::Endpoint endpoint = {
          .host = ep["host"].asString(),
          .socksPort = static_cast<uint16_t>(ep["socks"].asUInt()),
          .quicPort = static_cast<uint16_t>(ep["quic"].asUInt())
        };

        output.endpoints.emplace_back(std::move(endpoint));
      }
    }

    return true;
  }


  AsyncResultPtr<std::vector<RelayInfo>> BdKademlia::QueryRelays(const char * hints, size_t limit)
  {
    std::string query = "type:relay";
    if (hints)
    {
      query += std::string(" ") + hints;
    }

    BdObject::CArgs args;
    args["query"] = query;
    args["limit"] = Json::Value::UInt(limit);

    auto result = std::make_shared<AsyncResult<std::vector<RelayInfo>>>();

    bool rtn = this->Call("Query", args,
      [result](Json::Value & response, bool error)
      {
        result->SetError(error);
        if (error || !response.isArray())
        {
          result->Complete(std::vector<RelayInfo>());
        }
        else
        {
          std::vector<RelayInfo> regs;
          for (size_t i = 0; i < response.size(); ++i)
          {
            RelayInfo info;
            if (parseRelayInfo(response[i], info))
            {
              regs.emplace_back(std::move(info));
            }
          }

          result->Complete(std::move(regs));
        }
      }
    );

    return rtn ? result : nullptr;
  }
}
