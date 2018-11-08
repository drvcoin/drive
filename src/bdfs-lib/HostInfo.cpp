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

#include <json/json.h>
#include <drive/common/HostInfo.h>

namespace bdfs
{
  std::string HostInfo::ToString() const
  {
    Json::Value result;
    result["url"] = this->url;
    result["relays"] = Json::Value(Json::arrayValue);

    for (auto & relay : this->relays)
    {
      Json::Value relayObj;
      relayObj["name"] = relay.name;
      relayObj["endpoints"] = Json::Value(Json::arrayValue);

      for (auto & ep : relay.endpoints)
      {
        Json::Value endpoint;
        endpoint["host"] = ep.host;
        endpoint["socks"] = Json::UInt(ep.socksPort);
        endpoint["quic"] = Json::UInt(ep.quicPort);

        relayObj["endpoints"].append(endpoint);
      }

      result["relays"].append(relayObj);
    }

    Json::FastWriter writer;
    return writer.write(result);
  }


  bool HostInfo::FromString(const std::string & str)
  {
    Json::Value obj;

    Json::Reader reader;
    if (!reader.parse(str, obj) || 
        obj.isNull() || 
        !obj.isObject() ||
        !obj["url"].isString())
    {
      return false;
    }

    this->url = obj["url"].asString();

    if (!obj["relays"].isArray())
    {
      return true;
    }

    for (Json::Value::ArrayIndex i = 0; i != obj["relays"].size(); ++i)
    {
      Json::Value & relayObj = obj["relays"][i];
      if (!relayObj["name"].isString() || !relayObj["endpoints"].isArray())
      {
        continue;
      }

      RelayInfo relay;
      relay.name = relayObj["name"].asString();

      for (Json::Value::ArrayIndex j = 0; j != relayObj["endpoints"].size(); ++j)
      {
        Json::Value & epObj = relayObj["endpoints"][j];
        if (!epObj["host"].isString())
        {
          continue;
        }

        RelayInfo::Endpoint endpoint;
        endpoint.host = epObj["host"].asString();

        if (epObj["socks"].isIntegral())
        {
          endpoint.socksPort = static_cast<uint16_t>(epObj["socks"].asUInt());
        }

        if (epObj["quic"].isIntegral())
        {
          endpoint.quicPort = static_cast<uint16_t>(epObj["quic"].asUInt());
        }

        relay.endpoints.emplace_back(std::move(endpoint));
      }

      this->relays.emplace_back(std::move(relay));
    }

    return true;
  }
}
