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

#include "Volume.h"
#include "cm256.h"
#include "gf256.h"
#include "Util.h"

#include <memory.h>
#include <memory>
#include <openssl/sha.h>

namespace dfs
{
  Volume::Row::Row(Volume * volume, uint64_t row) :
    volume(volume),
    row(row)
  {
  }

  bool Volume::Row::Verify()
  {
    for (uint64_t i = 0; i < volume->Columns(); i++)
    {
      Partition * p = volume->partitions[i];
      return_false_if_msg(p == NULL, "Error: partition '%ld' is not set.\n", i);
      if (!p->VerifyBlock(row))
      {
        return_false_if(!Decode());
      }
    }
    return true;
  }

  bool Volume::Row::Decode()
  {
    size_t blockSize = volume->BlockSize();
    uint64_t codeCount = volume->CodeCount();
    uint64_t dataCount = volume->DataCount();

    cm256_encoder_params params;
    params.BlockBytes = blockSize;
    params.OriginalCount = dataCount;
    params.RecoveryCount = codeCount;

    size_t dataSize = dataCount * blockSize;
    std::unique_ptr<uint8_t[]> dataBuffer(new uint8_t[dataSize]);
    memset(dataBuffer.get(), 0, dataSize);

    std::vector<uint64_t> missingBlocks;

    cm256_block blocks[256] = {0};

    for (int i = 0; i < dataCount; i++)
    {
      uint8_t * dataCell = dataBuffer.get() + (i * blockSize);
      blocks[i].Block = dataCell;
      if (volume->__VerifyCell(row, i))
      {
        return_false_if(!volume->__ReadCached(row, i, dataCell, blockSize, 0));
        blocks[i].Index = i;
      }
      else
      {
        missingBlocks.push_back(i);
      }
    }

    if (missingBlocks.size() > 0)
    {
      size_t mi = 0;
      for (int i = 0; i < codeCount; i++)
      {
        if (volume->__VerifyCell(row, i+dataCount))
        {
          size_t oi = missingBlocks[mi++];
          return_false_if(!volume->__ReadCached(row, i+dataCount, blocks[oi].Block, blockSize, 0));
          blocks[oi].Index = dataCount + i;
          if (mi == missingBlocks.size())
          {
            return_false_if_msg(cm256_decode(params, blocks), "Error: failed to decode row '%lx'.\n", row);
            for (int j = 0; j < missingBlocks.size(); ++j)
            {
              oi = missingBlocks[j];
              printf("Recovered [%lx,%lx]\n", row, oi);
              volume->__WriteCached(row, oi, blocks[oi].Block, blockSize, 0);
            }
            break;
          }
        }
      }
    }

    size_t codeSize = codeCount * blockSize;
    std::unique_ptr<uint8_t[]> codeBuffer(new uint8_t[codeSize]);

    return_false_if(cm256_encode(params, blocks, codeBuffer.get()));

    for (int i = 0; i < codeCount; ++i)
    {
      uint8_t * codeCell = codeBuffer.get() + (i * blockSize);
      volume->__WriteCached(row, i+dataCount, codeCell, blockSize, 0);
    }

    return true;
  }

  bool Volume::Row::Encode()
  {
    size_t blockSize = volume->BlockSize();
    uint64_t codeCount = volume->CodeCount();
    uint64_t dataCount = volume->DataCount();

    cm256_encoder_params params;
    params.BlockBytes = blockSize;
    params.OriginalCount = dataCount;
    params.RecoveryCount = codeCount;

    size_t dataSize = dataCount * blockSize;
    std::unique_ptr<uint8_t[]> dataBuffer(new uint8_t[dataSize]);
    memset(dataBuffer.get(), 0, dataSize);

    cm256_block blocks[256];

    for (int i = 0; i < dataCount; i++)
    {
      uint8_t * dataCell = dataBuffer.get() + (i * blockSize);
      volume->__ReadCached(row, i, dataCell, blockSize, 0);
      blocks[i].Block = dataCell;
    }

    size_t codeSize = codeCount * blockSize;
    std::unique_ptr<uint8_t[]> codeBuffer(new uint8_t[codeSize]);

    return_false_if_msg(cm256_encode(params, blocks, codeBuffer.get()), "Error: erasure coding failed\n")

    for (int i = 0; i < params.RecoveryCount; ++i)
    {
      uint8_t * codeCell = codeBuffer.get() + (i * blockSize);
      volume->__WriteCached(row, i+dataCount, codeCell, blockSize, 0);
    }

    return true;
  }
}
