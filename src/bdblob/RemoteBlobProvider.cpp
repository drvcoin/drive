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


  std::unique_ptr<IBlob> RemoteBlobProvider::NewBlob(size_t size)
  {
    std::string id = uuidgen();
    std::string folder = this->rootPath + "/" + id;
    if (isFolderExist(folder.c_str()))
    {
      return nullptr;
    }

    if(dfs::VolumeManager::CreateVolume(id, size, bdblob::BlobConfig::DataBlocks(), bdblob::BlobConfig::CodeBlocks(), true))
    {
      std::string blobFileName = folder + "/blob";
      auto volume = dfs::VolumeManager::LoadVolume(id, "", true);
      if(volume != nullptr)
      {
        return std::unique_ptr<IBlob>(new RemoteBlob(std::move(volume), std::move(id), size));
      }
    }
    return nullptr;


    // mkdir(folder.c_str(), 0755);

    // std::string metaSize = folder + "/.size";
    // FILE * file = fopen(metaSize.c_str(), "wb");
    // if (file)
    // {
    //   fwrite(&size, 1, sizeof(size_t), file);
    //   fclose(file);
    // }

    // std::string blobFileName = folder + "/blob";
    // FILE * blobFile = fopen(blobFileName.c_str(), "wb");
    // if (blobFile)
    // {
    //   char buf[BUFSIZ];
    //   memset(buf, 0, BUFSIZ);

    //   size_t remaining = size;
    //   while (remaining >= BUFSIZ)
    //   {
    //     fwrite(buf, 1, BUFSIZ, blobFile);
    //     remaining -= BUFSIZ;
    //   }

    //   if (remaining > 0)
    //   {
    //     fwrite(buf, 1, remaining, blobFile);
    //   }

    //   fclose(blobFile);
    // }

    // return std::unique_ptr<IBlob>(new FileBlob(blobFileName.c_str(), std::move(id), size));
  }


  std::unique_ptr<IBlob> RemoteBlobProvider::OpenBlob(std::string id)
  {
    auto volume = dfs::VolumeManager::LoadVolume(id, "", true);
    if(volume.get() != nullptr)
    {
      auto dataSize = volume->DataSize();
      return std::unique_ptr<IBlob>(new RemoteBlob(std::move(volume), std::move(id), dataSize));
    } 
    
    return nullptr;
    // char path[PATH_MAX];
    // sprintf(path, "%s/%s/.size", this->rootPath.c_str(), id.c_str());
    
    // FILE * metaFile = fopen(path, "rb");
    // if (!metaFile)
    // {
    //   return nullptr;
    // }

    // size_t size = 0;
    // size_t bytes = fread(&size, 1, sizeof(size_t), metaFile);
    // fclose(metaFile);

    // if (bytes < sizeof(size_t))
    // {
    //   return nullptr;
    // }

    // sprintf(path, "%s/%s/blob", this->rootPath.c_str(), id.c_str());

    // std::unique_ptr<IBlob> result(new FileBlob(path, std::move(id), size));
    // if (!result->IsOpened())
    // {
    //   return nullptr;
    // }

    // return std::move(result);
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