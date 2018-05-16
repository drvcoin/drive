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

#include "BdPartition.h"

namespace dfs
{
  class Partition
  {
  public:

    Partition(std::shared_ptr<bdfs::BdPartition> obj, uint64_t blockCount, size_t blockSize);

    const uint64_t BlockCount() const { return blockCount; }
    const size_t BlockSize() const { return blockSize; }

    bool VerifyBlock(uint64_t index);
    bool InitBlock(uint64_t index);
    bool ReadBlock(uint64_t index, void * buffer, size_t size, size_t offset);
    bool WriteBlock(uint64_t index, const void * buffer, size_t size, size_t offset);

  private:

    uint64_t blockCount;

    size_t blockSize;

    std::shared_ptr<bdfs::BdPartition> ref;
  };
}
