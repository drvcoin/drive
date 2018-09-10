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
#include "BlobConfig.h"
#include "BlobProvider.h"
#include "File.h"


namespace bdblob
{
  File::File(BlobProvider * provider)
    : provider(provider)
    , blob(nullptr)
    , size(0)
  {
    assert(provider);
  }


  File::~File()
  {
    this->Close();
  }


  bool File::Open(uint64_t size)
  {
    if (this->IsOpened())
    {
      return false;
    }

    uint64_t capacity = this->size == 0 ? 1 : this->size;
    capacity = ((capacity - 1) / BlobConfig::BlockSize() + 1) * BlobConfig::BlockSize();

    this->blob = this->provider->NewBlob(capacity);
    if (!this->blob)
    {
      return false;
    }

    this->size = size;
    this->id = this->blob->Id();
    return true;
  }


  bool File::Open(std::string id, uint64_t size)
  {
    if (this->IsOpened())
    {
      return false;
    }

    auto blob = this->provider->OpenBlob(id);
    if (!blob || blob->Size() < size)
    {
      return false;
    }

    this->blob = std::move(blob);
    this->id = std::move(id);
    this->size = size;
    return true;
  }


  uint64_t File::Read(uint64_t offset, void * buffer, uint64_t len) const
  {
    if (!this->IsOpened())
    {
      return 0;
    }

    uint64_t start = std::min<uint64_t>(offset, this->size);
    uint64_t end = std::min<uint64_t>(offset + len, this->size);

    if (end <= start)
    {
      return 0;
    }

    return this->blob->Read(start, buffer, end - start);
  }


  uint64_t File::Write(uint64_t offset, const void * buffer, uint64_t len)
  {
    if (!this->IsOpened())
    {
      return 0;
    }

    uint64_t capacity = this->size == 0 ? 1 : this->size;
    capacity = ((capacity - 1) / BlobConfig::BlockSize() + 1) * BlobConfig::BlockSize();

    uint64_t start = std::min<uint64_t>(offset, capacity);
    uint64_t end = std::min<uint64_t>(offset + len, capacity);

    if (end <= start)
    {
      return 0;
    }

    uint64_t result = this->blob->Write(start, buffer, end - start);
    if (this->size < start + result)
    {
      this->size = start + result;
    }

    return result;
  }


  void File::Close()
  {
    this->blob.reset();
  }
}