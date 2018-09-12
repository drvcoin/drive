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

#include "FileBlob.h"
#include "BlobMetadata.h"
#include "FileBlobProvider.h"

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


  FileBlobProvider::FileBlobProvider(const char * rootPath)
    : rootPath(rootPath ? rootPath : ".")
  {
    if (this->rootPath != ".")
    {
      mkdir(this->rootPath.c_str(), 0755);
    }
  }


  std::unique_ptr<IBlob> FileBlobProvider::NewBlob(uint64_t size)
  {
    std::string id = uuidgen();

    if (!isFolderExist(this->rootPath.c_str()))
    {
      return nullptr;
    }

    auto blobMap = this->GetBlobMap();
    if (!blobMap)
    {
      return nullptr;
    }

    BlobMetadata metadata;
    metadata.SetSize(size);

    blobMap->SetMetadata(id, metadata);

    std::string blobFileName = this->rootPath + "/" + id;
    FILE * blobFile = fopen(blobFileName.c_str(), "wb");
    if (blobFile)
    {
      char buf[BUFSIZ];
      memset(buf, 0, BUFSIZ);

      size_t remaining = size;
      while (remaining >= BUFSIZ)
      {
        fwrite(buf, 1, BUFSIZ, blobFile);
        remaining -= BUFSIZ;
      }

      if (remaining > 0)
      {
        fwrite(buf, 1, remaining, blobFile);
      }

      fclose(blobFile);
    }

    return std::unique_ptr<IBlob>(new FileBlob(blobFileName.c_str(), std::move(id), size));
  }


  std::unique_ptr<IBlob> FileBlobProvider::OpenBlob(std::string id)
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

    char path[PATH_MAX];
    sprintf(path, "%s/%s", this->rootPath.c_str(), id.c_str());
    std::unique_ptr<IBlob> result(new FileBlob(path, std::move(id), metadata.Size()));
    if (!result->IsOpened())
    {
      return nullptr;
    }

    return std::move(result);
  }


  void FileBlobProvider::DeleteBlob(std::string id)
  {
    char path[PATH_MAX];

    sprintf(path, "%s/%s", this->rootPath.c_str(), id.c_str());
    unlink(path);
  }
}