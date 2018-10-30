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

#include <string.h>
#include <unistd.h>
#include "BufferedOutputStream.h"
#include "BufferedInputStream.h"
#include "BlobMap.h"

namespace bdblob
{
  BlobMap::~BlobMap()
  {
    if (this->file)
    {
      fclose(this->file);
      this->file = nullptr;

      unlink(this->filename.c_str());
    }
  }


  bool BlobMap::Initialize(std::string path)
  {
    if (this->file || path.empty())
    {
      return false;
    }

    this->file = fopen(path.c_str(), "wb+");
    if (!this->file)
    {
      return false;
    }

    this->filename = std::move(path);
    return true;
  }


  void BlobMap::SetValue(std::string key, const void * data, size_t len)
  {
    if (!this->file || key.empty() || !data || len == 0)
    {
      return;
    }

    Buffer buffer = GetValue(key);
    if (buffer.Size() == len && memcmp(buffer.Buf(), data, len) == 0)
    {
      return;
    }

    Index index = {0};
    if (fseeko(this->file, 0, SEEK_END) < 0)
    {
      return;
    }

    index.offset = ftello(this->file);
    index.size = fwrite(data, 1, len, this->file);
    if (index.size < len)
    {
      return;
    }

    this->indexes[key] = std::move(index);
  }


  Buffer BlobMap::GetValue(std::string key) const
  {
    if (!this->file || key.empty())
    {
      return Buffer();
    }

    auto itr = this->indexes.find(std::move(key));
    if (itr == this->indexes.end() || itr->second.size == 0)
    {
      return Buffer();
    }

    if (fseeko(this->file, itr->second.offset, SEEK_SET) < 0)
    {
      return Buffer();
    }

    Buffer result;
    if (!result.Resize(itr->second.size))
    {
      return Buffer();
    }

    if (fread(result.Buf(), 1, itr->second.size, this->file) < itr->second.size)
    {
      return Buffer();
    }

    return std::move(result);
  }


  void BlobMap::SetMetadata(std::string key, const BlobMetadata & metadata)
  {
    dfs::BufferedOutputStream stream;
    if (metadata.Serialize(stream))
    {
      this->SetValue(std::move(key), stream.Buffer(), stream.Offset());
    }
  }


  bool BlobMap::GetMetadata(std::string key, BlobMetadata & metadata) const
  {
    auto buffer = this->GetValue(std::move(key));
    if (buffer.Size() == 0)
    {
      return false;
    }

    dfs::BufferedInputStream stream{static_cast<const uint8_t *>(buffer.Buf()), buffer.Size()};
    return metadata.Deserialize(stream);
  }


  bool BlobMap::HasKey(std::string key) const
  {
    return this->indexes.find(std::move(key)) != this->indexes.end();
  }
}
