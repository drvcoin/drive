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
#include "Partition.h"

namespace dfs
{
  Partition::Partition(std::shared_ptr<bdfs::BdPartition> obj, uint64_t blockCount, size_t blockSize)
    : blockCount(blockCount)
    , blockSize(blockSize)
    , ref(obj)
  {
  }


  bool Partition::VerifyBlock(uint64_t index)
  {
    return true;
  }


  bool Partition::InitBlock(uint64_t index)
  {
    return true;
  }


  bool Partition::ReadBlock(uint64_t index, void * buffer, size_t size, size_t offset)
  {
    auto result = ref->Read(index, offset, size);
    if (result->Wait(5000))
    {
      auto & buf = result->GetResult();
      if (buf.size() == size)
      {
        memcpy(buffer, &buf[0], size);
        return true;
      }
    }

    return false;
  }


  bool Partition::WriteBlock(uint64_t index, const void * buffer, size_t size, size_t offset)
  {
    auto result = ref->Write(index, offset, buffer, size);
    if (result->Wait(5000))
    {
      return result->GetResult() == static_cast<ssize_t>(size);
    }

    return false;
  }


  bool Partition::Delete()
  {
    auto result = ref->Delete();
    if (result->Wait(5000))
    {
      return result->GetResult();
    }

    return false;
  }
}

