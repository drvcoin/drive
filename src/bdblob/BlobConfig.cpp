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
#include "BlobMap.h"
#include "BufferedOutputStream.h"
#include "BufferedInputStream.h"
#include "Buffer.h"
#include "BlobConfig.h"

namespace bdblob
{
  uint64_t BlobConfig::dataBlocks = 2;
  uint64_t BlobConfig::codeBlocks = 2;
  uint64_t BlobConfig::blockSize = 64 * 1024;

  std::string * BlobConfig::rootId = nullptr;

  void BlobConfig::SetRootId(std::string val, BlobProvider * provider)
  {
    auto blobMap = provider->GetBlobMap();
    assert(blobMap);

    // TODO: save volume.conf for root instead
    if (*rootId != val)
    {
      // TODO: move this into /etc/...
      FILE * file = fopen("blob_root", "w");
      if (file)
      {
        fwrite(val.c_str(), 1, val.size(), file);
        fclose(file);
      }

      file = fopen("blob_root.metadata", "wb");
      if (file)
      {
        BlobMetadata metadata;
        if (blobMap->GetMetadata(val, metadata))
        {
          dfs::BufferedOutputStream stream;
          if (metadata.Serialize(stream))
          {
            fwrite(stream.Buffer(), 1, stream.Offset(), file);
          }
        }

        fclose(file);
      }

      *rootId = std::move(val);
    }
  }


  static Buffer readFile(const char * path, const char * mode)
  {
    Buffer buffer;

    FILE * file = fopen(path, mode);
    if (file)
    {
      fseek(file, 0, SEEK_END);
      long size = ftell(file);
      rewind(file);

      if (buffer.Resize(size))
      {
        fread(buffer.Buf(), 1, size, file);
      }

      fclose(file);
    }

    return std::move(buffer);
  }


  void BlobConfig::Initialize(BlobProvider * provider)
  {
    auto blobMap = provider->GetBlobMap();
    assert(blobMap);

    rootId = new std::string();

    auto buffer = readFile("blob_root", "r");
    *rootId = std::string(static_cast<const char *>(buffer.Buf()), buffer.Size());

    buffer = readFile("blob_root.metadata", "rb");
    if (buffer.Size() > 0)
    {
      dfs::BufferedInputStream stream{static_cast<const uint8_t *>(buffer.Buf()), buffer.Size()};
      BlobMetadata metadata;
      if (metadata.Deserialize(stream))
      {
        blobMap->SetMetadata(*rootId, metadata);
      }
    }
  }
}
