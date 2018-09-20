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

#include <sys/stat.h>
#include <sys/types.h>
#include <uuid/uuid.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <cmath>

#include "RemoteBlob.h"
#include "RemoteBlobProvider.h"
#include "VolumeManager.h"
#include "BlobConfig.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

namespace bdblob
{
  static std::string uuidgen()
  {
    uuid_t obj;
    uuid_generate(obj);
    char str[36];
    uuid_unparse(obj, str);
    return str;
  }

  static bool isFolderExist(const char * path)
  {
    struct stat info;
    if (stat(path, &info) != 0)
    {
      return false;
    }

    return info.st_mode & S_IFDIR;
  }


  RemoteBlobProvider::RemoteBlobProvider(const char * rootPath)
    : rootPath(rootPath ? rootPath : ".")
  {
    if (this->rootPath != ".")
    {
      mkdir(this->rootPath.c_str(), 0755);
    }
  }


  std::unique_ptr<IBlob> RemoteBlobProvider::NewBlob(uint64_t size)
  {
    std::string id = uuidgen();
    std::string folder = this->rootPath + "/" + id;
    if (isFolderExist(folder.c_str()))
    {
      return nullptr;
    }

    auto blobMap = this->GetBlobMap();
    if (!blobMap)
    {
      return nullptr;
    }

    Json::Value volumeInfo = dfs::VolumeManager::CreateVolumePartitions(id, size, bdblob::BlobConfig::DataBlocks(), bdblob::BlobConfig::CodeBlocks());
    BlobMetadata metadata;
    metadata.SetSize(size);
    metadata.SetBlockSize(volumeInfo["blockSize"].asUInt());

    for (size_t i = 0; i < volumeInfo["partitions"].size(); ++i)
    {
      auto & partition = volumeInfo["partitions"][i];
      if (!partition["name"].isString() || !partition["provider"].isString())
      {
        return nullptr;
      }
      metadata.AddPartition(partition["name"].asString(), partition["provider"].asString());
    }

    blobMap->SetMetadata(id, metadata);

    if(!volumeInfo.isNull())
    {
      auto volume = dfs::VolumeManager::LoadVolumeFromJson(id, volumeInfo);
      if(volume != nullptr)
      {
        return std::unique_ptr<IBlob>(new RemoteBlob(std::move(volume), std::move(id), size));
      }
    }
    return nullptr;
  }


  std::unique_ptr<IBlob> RemoteBlobProvider::OpenBlob(std::string id)
  {
    auto blobMap = this->GetBlobMap();
    if (!blobMap)
    {
      return nullptr;
    }

    BlobMetadata metadata;
    if (!blobMap->GetMetadata(id, metadata))
    {
      return nullptr;
    }

    Json::Value partitionArray;
    for(auto & partition : metadata.Partitions())
    {
        Json::Value pt;
        pt["name"] = partition.name;
        pt["provider"] = partition.provider;
        partitionArray.append(pt);
    }

    Json::Value json;
    json["partitions"] = partitionArray;
    json["blockSize"] = Json::Value::UInt(metadata.BlockSize());
    json["blockCount"] = Json::Value::UInt(std::ceil((metadata.Size()/bdblob::BlobConfig::DataBlocks()) * 1.0  / metadata.BlockSize()));
    json["dataBlocks"] = Json::Value::UInt(bdblob::BlobConfig::DataBlocks());
    json["codeBlocks"] = Json::Value::UInt(bdblob::BlobConfig::CodeBlocks());

    auto volume = dfs::VolumeManager::LoadVolumeFromJson(id, json);
    if(volume.get() != nullptr)
    {
      auto dataSize = volume->DataSize();
      return std::unique_ptr<IBlob>(new RemoteBlob(std::move(volume), std::move(id), dataSize));
    } 
    
    return nullptr;
  }


  void RemoteBlobProvider::DeleteBlob(std::string id)
  {
    char path[PATH_MAX];

    sprintf(path, "%s/%s/blob", this->rootPath.c_str(), id.c_str());
    unlink(path);

    sprintf(path, "%s/%s/.size", this->rootPath.c_str(), id.c_str());
    unlink(path);

    sprintf(path, "%s/%s", this->rootPath.c_str(), id.c_str());
    unlink(path);
  }
}
