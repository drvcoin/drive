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

#include <stdint.h>
#include <stddef.h>

#include "Partition.h"

#include <string>
#include <vector>
#include <memory>

#include <openssl/aes.h>

namespace dfs
{
  class Cache;

  class Volume
  {
    class Cell
    {
    private:
      Volume * volume;
      uint64_t row;
      uint64_t column;
    public:
      Cell(Volume * volume, uint64_t row, uint64_t column);
      bool Verify();
      bool Init();
      bool Write(const void * buffer, size_t size, size_t offset);
      bool Read(void * buffer, size_t size, size_t offset);
    };

    class Row
    {
    private:
      Volume * volume;
      uint64_t row;
    public:
      Row(Volume * volume, uint64_t row);
      bool Verify();
      bool Decode();
      bool Encode();
    };

    class Column
    {
    private:
      Volume * volume;
      uint64_t column;
    public:
      Column(Volume * volume, uint64_t column);
      bool Init();
      Cell GetCell(uint64_t row);
    };

    friend class Cell;
    friend class Row;
    friend class Column;

  private:
    uint8_t * zeroBuffer;
    std::string volumeId;
    uint64_t blockCount;
    uint64_t dataCount;
    uint64_t codeCount;
    size_t blockSize;
    std::vector<Partition*> partitions;
    AES_KEY encryptKey;
    AES_KEY decryptKey;

    std::unique_ptr<Cache> cache;

  public:
    Volume(const char * volumeId, uint64_t dataCount, uint64_t codeCount, uint64_t blockCount, size_t blockSize, const char * password);
    ~Volume();

    bool SetPartition(uint64_t index, Partition * partition);

    void EnableCache(std::unique_ptr<Cache> cache);

    const uint64_t Rows() { return blockCount; }
    const uint64_t Columns() { return partitions.size(); }
    const uint64_t DataCount() { return dataCount; }
    const uint64_t CodeCount() { return codeCount; }
    const uint64_t BlockCount() { return blockCount; }
    const size_t BlockSize() { return blockSize; }

    const size_t DataSize() { return dataCount * blockCount * blockSize; }
    const size_t CodeSize() { return codeCount * blockCount * blockSize; }
    const size_t TotalSize() { return (dataCount + codeCount) * blockCount * blockSize; }

    const uint8_t * GetZeroBuffer();

    Volume::Column GetColumn(uint64_t column);
    Volume::Row GetRow(uint64_t row);
    Volume::Cell GetCell(uint64_t row, uint64_t column);

    bool WriteEncrypt(const void * buffer, size_t size, size_t offset);
    bool ReadDecrypt(void * buffer, size_t size, size_t offset);

    bool Write(const void * buffer, size_t size, size_t offset);
    bool Read(void * buffer, size_t size, size_t offset);

    bool Delete();

    bool __VerifyCell(uint64_t row, uint64_t column);
    bool __WriteCell(uint64_t row, uint64_t column, const void * buffer, size_t size, size_t offset);
    bool __ReadCell(uint64_t row, uint64_t column, void * buffer, size_t size, size_t offset);

    bool __ReadCached(uint64_t row, uint64_t column, void * buffer, size_t size, size_t offset);
    bool __WriteCached(uint64_t row, uint64_t column, const void * buffer, size_t size, size_t offset);

    bool __ReadDirect(uint64_t row, uint64_t column, void * buffer, size_t size, size_t offset);
    bool __WriteDirect(uint64_t row, uint64_t column, const void * buffer, size_t size, size_t offset);
  };
}
