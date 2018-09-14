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

#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include "IOutputStream.h"
#include "IInputStream.h"

namespace bdblob
{
  class BlobMetadata
  {
  public:

    struct PartitionInfo
    {
      std::string name;
      std::string provider;
    };

  public:

    bool Serialize(IOutputStream & stream) const;

    bool Deserialize(IInputStream & stream);

    size_t GetSerializedSize() const;

    void AddPartition(std::string name, std::string provider);

    uint64_t Size() const                                     { return this->size; }

    void SetSize(uint64_t val)                                { this->size = val; }

    uint64_t BlockSize() const                                     { return this->blockSize; }

    void SetBlockSize(uint64_t val)                                { this->blockSize = val; }

    const std::vector<PartitionInfo> & Partitions() const     { return this->partitions; }

  private:

    uint64_t size = 0;

    uint64_t blockSize = 0;

    std::vector<PartitionInfo> partitions;
  };
}