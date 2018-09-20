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
#include <string>
#include <vector>
#include <fstream>
#include "BlobConfig.h"
#include "FileBlobProvider.h"
#include "RemoteBlobProvider.h"
#include "BlobApi.h"
#include "VolumeManager.h"
#include "commands/CatCommand.h"
#include "commands/GetCommand.h"
#include "commands/ListCommand.h"
#include "commands/MkdirCommand.h"
#include "commands/MvCommand.h"
#include "commands/PutCommand.h"
#include "commands/RmCommand.h"

namespace bdblob
{
  BlobApi * g_api = nullptr;
}


static void PrintUsage(const char * prefix)
{
  printf("Usage: %s <action>\n", prefix);
  printf("\n");
  printf("Actions:\n");
  printf("  cat\t\t\tShow file content.\n");
  printf("  get\t\t\tDownload file.\n");
  printf("  ls\t\t\tList files or directories.\n");
  printf("  mkdir\t\t\tCreate directory.\n");
  printf("  mv\t\t\tMove file or directory.\n");
  printf("  rm\t\t\tDelete files or directories.\n");
  printf("  put\t\t\tUpload file.\n");
}


void ReadConfig()
{
  std::ifstream cfg("/etc/drive/bdblob.conf");
  std::string data((std::istreambuf_iterator<char>(cfg)),
               std::istreambuf_iterator<char>());

  if(!data.size())
  {
    return;
  }

  Json::Reader reader;
  Json::Value json;
  if (!reader.parse(data.c_str(), data.size(), json, false) ||
      !json.isObject())
  {
    return;
  }

  if(json["kademlia"].isArray())
  {
    for(int i = 0; i < json["kademlia"].size(); i++)
    {
      auto kd = json["kademlia"][i].asString();
      dfs::VolumeManager::kademliaUrl.emplace_back(kd);
    }
  }
}


int main(int argc, const char ** argv)
{
  if (argc < 2)
  {
    PrintUsage(argv[0]);
    return -1;
  }

  ReadConfig();

  if(dfs::VolumeManager::kademliaUrl.size() == 0)
  {
      dfs::VolumeManager::kademliaUrl.emplace_back("http://18.220.231.21:7800");
  }

  std::unique_ptr<bdblob::BlobProvider> provider = nullptr;

#ifdef LOCAL_BLOB
  provider = std::make_unique<bdblob::FileBlobProvider>("storage");
#else
  provider = std::make_unique<bdblob::RemoteBlobProvider>(".");
#endif

  provider->InitializeBlobMap("blob.cache");

  bdblob::BlobConfig::Initialize(provider.get());

  bdblob::g_api = new bdblob::BlobApi();
  bdblob::g_api->Initialize(std::move(provider), bdblob::BlobConfig::RootId());

  std::vector<std::unique_ptr<bdblob::Command>> commands;
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::CatCommand(argv[0])));
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::GetCommand(argv[0])));
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::ListCommand(argv[0])));
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::MkdirCommand(argv[0])));
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::MvCommand(argv[0])));
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::PutCommand(argv[0])));
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::RmCommand(argv[0])));


  int retval = -1;

  for (const auto & command : commands)
  {
    if (command->Name() == argv[1])
    {
      retval = command->Execute(argc - 1, &argv[1]);
      break;
    }
  }

  dfs::VolumeManager::Stop();

  if(retval == -1)
  {
    PrintUsage(argv[0]);
  }

  return retval;
}
