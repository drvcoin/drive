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
#include <stdint.h>

#include "VolumeManager.h"
#include "BdTypes.h"
#include "BdSession.h"

#include "cm256.h"
#include "gf256.h"

#include "BdKademlia.h"
#include "BdPartitionFolder.h"
#include "Buffer.h"
#include "ContractRepository.h"

namespace dfs
{
	bdfs::HttpConfig VolumeManager::defaultConfig;

  std::string VolumeManager::kademliaUrl;

	std::unique_ptr<Volume> VolumeManager::LoadVolume(const std::string & name)
	{
		FILE * file = fopen((name + ".config").c_str(), "r");
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
      if (ep.empty())
      {
        return nullptr;
      }

			auto session = bdfs::BdSession::CreateSession(ep.c_str(), &defaultConfig);
			auto name = config["name"].asString();
			auto path = "host://Partitions/" + name;
			auto partition = std::static_pointer_cast<bdfs::BdPartition>(session->CreateObject("Partition", path.c_str(), name.c_str()));

			volume->SetPartition(i, new Partition(partition, blockCount, blockSize));
		}

		return volume;
	}


  std::string VolumeManager::GetProviderEndpoint(const std::string & name)
  {
    if (kademliaUrl.empty())
    {
      return std::string();
    }

    auto session = bdfs::BdSession::CreateSession(kademliaUrl.c_str(), &defaultConfig);
    auto kademlia = std::static_pointer_cast<bdfs::BdKademlia>(
      session->CreateObject("Kademlia", "host://Kademlia", "Kademlia"));
    auto result = kademlia->GetValue(("ep:" + name).c_str());

    if (!result->Wait())
    {
      printf("Failed to connect to kademlia.\n");
      return std::string();
    }

    auto & buffer = result->GetResult();
    if (buffer.Size() == 0)
    {
      printf("Failed to find endpoint for provider '%s'\n", name.c_str());
      return std::string();
    }

    return std::string(static_cast<const char *>(buffer.Buf()), buffer.Size());    
  }


	bool VolumeManager::CreateVolume(const std::string & volumeName, const std::string & repoName, const uint16_t dataBlocks, const uint16_t codeBlocks)
	{
		std::vector<std::unique_ptr<bdcontract::Contract>> contracts;
		uint64_t size = std::numeric_limits<uint64_t>::max();

		size_t blockSize = 64*1024;

		bdcontract::ContractRepository repo{repoName.c_str()};

		for (const auto & name : repo.GetContractNames())
		{
			auto ptr = repo.LoadContract(name.c_str());
			if (ptr && ptr->Size() > blockSize)
			{
				size = std::min(ptr->Size(), size);
				contracts.emplace_back(std::move(ptr));
			}
		}

		if (contracts.size() < dataBlocks + codeBlocks)
		{
			printf("Not enough contract.\n");
			return false;
		}

		Json::Value arr;

		for (size_t i = 0; i < dataBlocks + codeBlocks; ++i)
		{
			auto ep = GetProviderEndpoint(contracts[i]->Provider());
			if (ep.empty())
			{
				return false;
			}
			auto session = bdfs::BdSession::CreateSession(ep.c_str(), &defaultConfig);
			auto folder = std::static_pointer_cast<bdfs::BdPartitionFolder>(
				session->CreateObject("PartitionFolder", "host://Partitions", "Partitions"));
			auto result = folder->CreatePartition(contracts[i]->Name().c_str(), blockSize);
			if (!result->Wait(5000) || !result->GetResult())
			{
				printf("Failed to create on partition from contract '%s'\n", contracts[i]->Name().c_str());
				return false;
			}

			auto obj = result->GetResult();

			Json::Value partition;
			partition["name"] = obj->Name();
			partition["provider"] = contracts[i]->Provider();

			arr.append(partition);
		}
		
		Json::Value volume;
		volume["blockSize"] = Json::Value::UInt(blockSize);
		volume["blockCount"] = Json::Value::UInt(size / blockSize);
		volume["dataBlocks"] = Json::Value::UInt(dataBlocks);
		volume["codeBlocks"] = Json::Value::UInt(codeBlocks);
		volume["partitions"] = arr;

		std::string result = volume.toStyledString();

		FILE * file = fopen((volumeName + ".config").c_str(), "w");
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

		_unlink((name + ".config").c_str());

    return true;
	}
}
