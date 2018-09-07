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

#define NOMINMAX
#include <stdio.h>
#include <sys/stat.h>
#include <cmath>
#include <limits>
#include <algorithm>
#include <json/json.h>
#include <thread>
#include <unordered_set>

#include "VolumeManager.h"
#include "BdTypes.h"
#include "BdSession.h"

#include "cm256.h"
#include "gf256.h"
#include "Util.h"
#include "BdKademlia.h"
#include "BdPartitionFolder.h"
#include "Buffer.h"
#include "ContractRepository.h"
#include "Paths.h"

namespace dfs
{
  bdfs::HttpConfig VolumeManager::defaultConfig;

  std::vector<std::string> VolumeManager::kademliaUrl;

  std::unique_ptr<Volume> VolumeManager::LoadVolume(const std::string & name)
  {
    std::string path = GetWorkingDir() + SLASH + name + SLASH + "volume.conf";
    printf("path=%s\n", path.c_str());
    FILE * file = fopen(path.c_str(), "r");
    if (!file)
    {
      return nullptr;
    }

    bdfs::Buffer buffer;
    buffer.Resize(BUFSIZ);
    size_t offset = 0;

    size_t bytes;
    while ((bytes = fread(static_cast<char *>(buffer.Buf()) + offset, 1, BUFSIZ, file)) == BUFSIZ)
    {
      offset = buffer.Size();
      buffer.Resize(buffer.Size() + BUFSIZ);
    }

    fclose(file);

    Json::Reader reader;
    Json::Value json;
    if (!reader.parse(static_cast<const char *>(buffer.Buf()), offset + bytes, json, false) ||
        !json.isObject() ||
        !json["blockSize"].isIntegral() ||
        !json["blockCount"].isIntegral() ||
        !json["dataBlocks"].isIntegral() ||
        !json["codeBlocks"].isIntegral() ||
        !json["partitions"].isArray())
    {
      return nullptr;
    }

    size_t blockSize = json["blockSize"].asUInt();
    uint64_t blockCount = json["blockCount"].asUInt();
    uint64_t dataBlocks = json["dataBlocks"].asUInt();
    uint64_t codeBlocks = json["codeBlocks"].asUInt();

    if (json["partitions"].size() < dataBlocks + codeBlocks)
    {
      return nullptr;
    }

    auto volume = std::make_unique<Volume>(name.c_str(), dataBlocks, codeBlocks, blockCount, blockSize, "HelloWorld");

    for (size_t i = 0; i < json["partitions"].size(); ++i)
    {
      auto & config = json["partitions"][i];
      if (!config["name"].isString() || !config["provider"].isString())
      {
        return nullptr;
      }

      auto ep = GetProviderEndpoint(config["provider"].asString());
      if (ep.url.empty())
      {
        return nullptr;
      }

      auto cfg = new bdfs::HttpConfig();
      cfg->Relays(std::move(ep.relays));
      auto session = bdfs::BdSession::CreateSession(ep.url.c_str(), cfg, true);
      auto name = config["name"].asString();
      auto path = "host://Partitions/" + name;
      auto partition = std::static_pointer_cast<bdfs::BdPartition>(session->CreateObject("Partition", path.c_str(), name.c_str()));

      volume->SetPartition(i, new Partition(partition, blockCount, blockSize));
    }

    return volume;
  }


  bdfs::HostInfo VolumeManager::GetProviderEndpoint(const std::string & name)
  {
    bdfs::HostInfo hostInfo;

    if (kademliaUrl.size() == 0)
    {
      return hostInfo;
    }

    for(auto &kd : kademliaUrl)
    {
      auto session = bdfs::BdSession::CreateSession(kd.c_str(), &defaultConfig);
      auto kademlia = std::static_pointer_cast<bdfs::BdKademlia>(
        session->CreateObject("Kademlia", "host://Kademlia", "Kademlia"));
      auto result = kademlia->GetValue(("ep:" + name).c_str());

      if (!result->Wait() || result->HasError())
      {
        printf("Failed to connect to kademlia.\n");
        continue;
      }

      auto & buffer = result->GetResult();
      if (buffer.Size() == 0)
      {
        printf("Failed to find endpoint for provider '%s'\n", name.c_str());
        return hostInfo;
      }

      std::string hostInfoStr = std::string(static_cast<const char *>(buffer.Buf()), buffer.Size());
      hostInfo.FromString(hostInfoStr);
      return hostInfo;
    }

    return hostInfo;
  }


  bool VolumeManager::CreateVolume(const std::string & volumeName, const uint64_t size, const uint16_t dataBlocks, const uint16_t codeBlocks)
  {
    size_t blockSize = 64*1024;
    auto providerSize = size / dataBlocks;
    auto providerCount = dataBlocks + codeBlocks;
    std::unordered_set<std::string> providersUsed;
    Json::Value partitionsArray;

    std::string query = "type:\"storage\" availableSize:" + std::to_string(providerSize);

    for(auto &kd : kademliaUrl)
    {
      auto session = bdfs::BdSession::CreateSession(kd.c_str(), &defaultConfig);
      auto kademlia = std::static_pointer_cast<bdfs::BdKademlia>(
        session->CreateObject("Kademlia", "host://Kademlia", "Kademlia"));

      int retryCount = 1;
      bool kadActive = false;
      do
      {
        auto qresult = kademlia->QueryStorage(query.c_str(), providerCount * retryCount);

        if (!qresult->Wait() || qresult->HasError())
        {
          printf("Failed to connect to kademlia.\n");
          break;
        }

        kadActive = true;

        auto &jsonArray = qresult->GetResult();
        if (jsonArray.isNull())
        {
          printf("Failed to query for providers.\n");
          return false;
        }

        std::vector<std::unique_ptr<bdcontract::Contract>> contracts;
        for(auto &json : jsonArray)
        {
          auto contract = std::make_unique<bdcontract::Contract>();
          contract->SetName(json["contract"].asString());
          contract->SetProvider(json["name"].asString());
          contract->SetSize(json["availableSize"].asUInt());
          contract->SetReputation(json["reputation"].asUInt());
          contracts.emplace_back(std::move(contract));
        }

        for (size_t i = 0; i < contracts.size(); ++i)
        {
          if(providersUsed.find(contracts[i]->Provider()) != providersUsed.end())
          {
            continue;
          }

          auto ep = GetProviderEndpoint(contracts[i]->Provider());
          if (ep.url.empty())
          {
            continue;
          }

          auto cfg = new bdfs::HttpConfig();
          cfg->Relays(std::move(ep.relays));
          providersUsed.emplace(contracts[i]->Provider());

          auto session = bdfs::BdSession::CreateSession(ep.url.c_str(), cfg, true);
          auto folder = std::static_pointer_cast<bdfs::BdPartitionFolder>(
            session->CreateObject("PartitionFolder", "host://Partitions", "Partitions"));

          std::string reserveId = "";
          auto reserveResult = folder->ReservePartition(providerSize);
          if (reserveResult->Wait(folder->GetTimeout()))
          {
            auto &buf = reserveResult->GetResult();
            if(buf.size() > 0)
            {
              reserveId = buf;
            }
          }

          if(reserveId == "")
          {
            printf("Failed to reserve space on provider '%s'\n", contracts[i]->Provider().c_str());
            continue;
          }

          auto result = folder->CreatePartition(reserveId, blockSize);
          if (!result->Wait(folder->GetTimeout()) || !result->GetResult())
          {
            continue;
          }

          auto obj = result->GetResult();

          Json::Value partition;
          partition["name"] = obj->Name();
          partition["provider"] = contracts[i]->Provider();

          partitionsArray.append(partition);

          if(partitionsArray.size() == providerCount)
          {
            break;
          }
        }

      } while(++retryCount <= 3 && partitionsArray.size() != providerCount);

      if(kadActive)
      {
        break;
      }
    }

    if(partitionsArray.size() != providerCount)
    {
      printf("Failed to reserve and create partition on desired providers\n");
      //TODO: Unreserve/delete partitions from the partial providers used
      return false;
    }

    Json::Value volume;
    volume["blockSize"] = Json::Value::UInt(blockSize);
    volume["blockCount"] = Json::Value::UInt(providerSize / blockSize);
    volume["dataBlocks"] = Json::Value::UInt(dataBlocks);
    volume["codeBlocks"] = Json::Value::UInt(codeBlocks);
    volume["partitions"] = partitionsArray;

    std::string result = volume.toStyledString();

    std::string path = GetWorkingDir() + SLASH + volumeName;

    MakeDir(path);
    //mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    path += SLASH + std::string("volume.conf");

    FILE * file = fopen(path.c_str(), "w");
    if (!file)
    {
      printf("Failed to create volume config file.\n");
      return false;
    }

    fwrite(result.c_str(), 1, result.size(), file);

    fclose(file);

    return true;
  }


  bool VolumeManager::DeleteVolume(const std::string & name)
  {
    auto volume = LoadVolume(name);
    if (!volume)
    {
      printf("Failed to load volume.\n");
      return false;
    }

    if (!volume->Delete())
    {
      printf("Failed to delete volume.\n");
      return false;
    }

    std::string path = GetWorkingDir() + SLASH + name + SLASH + "volume.conf";
    _unlink(path.c_str());

    return true;
  }
}
