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
#include <sys/stat.h>
#include <cmath>
#include <limits>
#include <unistd.h>
#include <json/json.h>
#include <thread>
#include "ActionHandler.h"
#include "BdTypes.h"
#include "BdSession.h"

#include "cm256.h"
#include "gf256.h"
#include "ubd.h"

#include "BdPartitionFolder.h"
#include "Buffer.h"
#include "ContractRepository.h"

namespace dfs
{
	bdfs::HttpConfig ActionHandler::defaultConfig;
  std::map<std::string, VolumeMeta *> ActionHandler::volumeInfo;
  std::map<std::string, bool> ActionHandler::nbdInfo = {
    {"/dev/nbd0",false},
    {"/dev/nbd1",false},
    {"/dev/nbd2",false},
    {"/dev/nbd3",false}
  };	
	// ubd callbacks	
	static size_t xmp_read(void *buf, size_t size, size_t offset, void * context)
	{
		return ((Volume*)context)->ReadDecrypt(buf, size, offset) ? 0 : -1;
	}

	static size_t xmp_write(const void *buf, size_t size, size_t offset, void * context)
	{
		return ((Volume*)context)->WriteEncrypt(buf, size, offset) ? 0 : -1;
	}

	static void xmp_disc(void * context)
	{
		(void)(context);
    printf("\n----------- Disc ------------ \n");
	}

	static int xmp_flush(void * context)
	{
		(void)(context);
    printf("\n----------- Flush ------------ \n");
		return 0;
	}

	static int xmp_trim(size_t from, size_t len, void * context)
	{
		(void)(context);
    printf("\n----------- Trim ------------ \n");
		return 0;
	}

	std::unique_ptr<Volume> ActionHandler::LoadVolume(const std::string & name)
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

			auto session = bdfs::BdSession::CreateSession(config["provider"].asCString(), &defaultConfig);
			auto name = config["name"].asString();
			auto path = "host://Partitions/" + name;
			auto partition = std::static_pointer_cast<bdfs::BdPartition>(session->CreateObject("Partition", path.c_str(), name.c_str()));

			volume->SetPartition(i, new Partition(partition, blockCount, blockSize));
		}

		return volume;
	}

  bool ActionHandler::MountVolume(const std::string &name, const std::string &path)
  {
    std::string nbdPath = ActionHandler::GetNextNBD();
    if(nbdPath == "") 
    {
      printf("No available block devices.\n");
      return false;
    }

    std::thread th([=]{

      printf("Processing: %s\n", nbdPath.c_str());

#if 0
      uint64_t blockCount = 256;//*1024*1024ul;
      size_t blockSize = 1*1024*1024;
      std::string volumeId = path;
#endif

      auto volume = ActionHandler::LoadVolume(name);

      static struct ubd_operations ops = {
        .read = xmp_read,
        .write = xmp_write,
        .disc = xmp_disc,
        .flush = xmp_flush,
        .trim = xmp_trim
      };


      ubd_register(nbdPath.c_str(), volume->DataSize(), &ops, (void *)volume.get());
    });
    th.detach();

    return true;
  }


	bool ActionHandler::CreateVolume(const std::string & volumeName, const std::string & repoName, const uint16_t dataBlocks, const uint16_t codeBlocks)
	{
		std::vector<std::unique_ptr<bdcontract::Contract>> contracts;

		uint64_t size = std::numeric_limits<uint64_t>::max();

		size_t blockSize = 1*1024*1024;

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
			auto session = bdfs::BdSession::CreateSession(contracts[i]->Provider().c_str(), &defaultConfig);
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


	bool ActionHandler::DeleteVolume(const std::string & name)
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

		unlink((name + ".config").c_str());

    return true;
	}

  std::string ActionHandler::GetNextNBD()
  {
    for(auto it : ActionHandler::nbdInfo)
    {
      if(!it.second)
        return it.first;
    }
    return "";
  }
}
