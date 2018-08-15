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
#include "Util.h"
#include "Cache.h"

#include <memory.h>
#include <memory>
#include <openssl/sha.h>

namespace dfs
{
  Volume::Volume(const char * volumeId, uint64_t dataCount, uint64_t codeCount, uint64_t blockCount, size_t blockSize, const char * password) :
    zeroBuffer(NULL),
    volumeId(volumeId),
    blockCount(blockCount),
    dataCount(dataCount),
    codeCount(codeCount),
    blockSize(blockSize),
    partitions(dataCount+codeCount)
  {
    if (password != NULL)
    {
      uint8_t key[SHA256_DIGEST_LENGTH];
      SHA256_CTX sha256;
      SHA256_Init(&sha256);
      SHA256_Update(&sha256, password, strlen(password));
      SHA256_Final(key, &sha256);
      AES_set_encrypt_key(key, 128, &(encryptKey));
      AES_set_decrypt_key(key, 128, &(decryptKey));
    }
  }

  Volume::~Volume()
  {
    this->cache.reset();
    for (std::vector<Partition*>::iterator i = partitions.begin(); i != partitions.end(); i++)
    {
      delete(*i);
    }
  }


  void Volume::EnableCache(std::unique_ptr<Cache> val)
  {
    this->cache = std::move(val);
  }


  bool Volume::SetPartition(uint64_t index, Partition * partition)
  {
    if (partition == NULL ||
        index > partitions.size() ||
        partition->BlockSize() != blockSize ||
        partition->BlockCount() < blockCount
      )
    {
      return false;
    }

    Partition * oldPartition = partitions[index];

    partitions[index] = partition;

    if (oldPartition != NULL)
    {
      // need to rebuild.
      delete(oldPartition);
    }

    return true;
  }

  
  uint32_t Volume::GetTimeout() const
  {
    uint32_t result = 0;
    for (auto partition : this->partitions)
    {
      result += partition->GetTimeout();
    }

    return result;
  }


  const uint8_t * Volume::GetZeroBuffer()
  {
    if (zeroBuffer == NULL)
    {
      zeroBuffer = new uint8_t[blockSize];
    }
    return zeroBuffer;
  }

  Volume::Column Volume::GetColumn(uint64_t column)
  {
    return Column(this, column);
  }

  Volume::Row Volume::GetRow(uint64_t row)
  {
    return Row(this, row);
  }

  Volume::Cell Volume::GetCell(uint64_t row, uint64_t column)
  {
    return Cell(this, row, column);
  }

  bool Volume::WriteEncrypt(const void * buffer, size_t size, size_t offset)
  {
    if (size == 0) { return true; }

    std::unique_ptr<uint8_t> clearBuffer(new uint8_t[blockSize]);
    std::unique_ptr<uint8_t> cryptBuffer(new uint8_t[blockSize]);
    uint64_t dataBlock = (uint64_t)(offset / blockSize);
    size_t blockOffset = offset - (dataBlock * blockSize);
    uint64_t row = dataBlock / dataCount;
    uint64_t col = dataBlock - (row * dataCount);
    size_t blockRemaining = blockSize - blockOffset;
    uint8_t * byteBuffer = (uint8_t*)buffer;

    return_false_if_msg(offset >= (blockCount*dataCount*blockSize), "Error: param 'offset' out of range: %ld\n", offset);
    return_false_if_msg(size > (blockCount*dataCount*blockSize), "Error: param 'size' out of range: %ld\n", offset);
    return_false_if_msg((offset+size) > (blockCount*dataCount*blockSize), "Error: param 'offset+size' out of range: %ld\n", offset+size);

    return_false_if_msg(!GetRow(row).Verify(), "Error: row '%lx' is corrupt.\n", row);

    uint8_t iv[AES_BLOCK_SIZE];

    while (true)
    {
      size_t toWrite = (size>blockRemaining)?blockRemaining:size;
      if (toWrite < blockSize)
      {
        return_false_if_msg(!__ReadCached(row, col, cryptBuffer.get(), blockSize, 0), "Error: failed to write [%lx,%lx].\n", row, col);
        memset(iv, row, AES_BLOCK_SIZE);
        AES_cbc_encrypt(cryptBuffer.get(), clearBuffer.get(), blockSize, &decryptKey, iv, AES_DECRYPT);
      }
      memcpy(clearBuffer.get() + blockOffset, byteBuffer, toWrite);
      memset(iv, row, AES_BLOCK_SIZE);
      AES_cbc_encrypt(clearBuffer.get(), cryptBuffer.get(), blockSize, &encryptKey, iv, AES_ENCRYPT);
      return_false_if_msg(!__WriteCached(row, col, cryptBuffer.get(), blockSize, 0), "Error: failed to write [%lx,%lx].\n", row, col);
      byteBuffer += toWrite;
      size -= toWrite;
      blockRemaining = blockSize;
      blockOffset = 0;

      if (size == 0) { break; }

      if (++col == dataCount)
      {
        return_false_if_msg(!GetRow(row).Encode(), "Error: row '%lx' could not be encoded.\n", row);

        col = 0;
        row++;

        return_false_if_msg(!GetRow(row).Verify(), "Error: row '%lx' is corrupt.\n", row);
      }
    }

    return_false_if_msg(!GetRow(row).Encode(), "Error: row '%lx' could not be encoded.\n", row);

    return true;
  }

  bool Volume::Write(const void * buffer, size_t size, size_t offset)
  {
    if (size == 0) { return true; }

    uint64_t dataBlock = (uint64_t)(offset / blockSize);
    size_t blockOffset = offset - (dataBlock * blockSize);
    uint64_t row = dataBlock / dataCount;
    uint64_t col = dataBlock - (row * dataCount);
    size_t blockRemaining = blockSize - blockOffset;
    uint8_t * byteBuffer = (uint8_t*)buffer;

    return_false_if_msg(offset >= (blockCount*dataCount*blockSize), "Error: param 'offset' out of range: %ld\n", offset);
    return_false_if_msg(size > (blockCount*dataCount*blockSize), "Error: param 'size' out of range: %ld\n", offset);
    return_false_if_msg((offset+size) > (blockCount*dataCount*blockSize), "Error: param 'offset+size' out of range: %ld\n", offset+size);

    return_false_if_msg(!GetRow(row).Verify(), "Error: row '%lx' is corrupt.\n", row);

    while (true)
    {
      size_t toWrite = (size>blockRemaining)?blockRemaining:size;
      return_false_if_msg(!__WriteCached(row, col, byteBuffer, toWrite, blockOffset), "Error: failed to write [%lx,%lx].\n", row, col);
      byteBuffer += toWrite;
      size -= toWrite;
      blockRemaining = blockSize;
      blockOffset = 0;

      if (size == 0) { break; }

      if (++col == dataCount)
      {
        return_false_if_msg(!GetRow(row).Encode(), "Error: row '%lx' could not be encoded.\n", row);

        col = 0;
        row++;

        return_false_if_msg(!GetRow(row).Verify(), "Error: row '%lx' is corrupt.\n", row);
      }
    }

    return_false_if_msg(!GetRow(row).Encode(), "Error: row '%lx' could not be encoded.\n", row);

    return true;
  }

  bool Volume::ReadDecrypt(void * buffer, size_t size, size_t offset)
  {
    if (size == 0) { return true; }

    std::unique_ptr<uint8_t> clearBuffer(new uint8_t[blockSize]);
    std::unique_ptr<uint8_t> cryptBuffer(new uint8_t[blockSize]);
    uint64_t dataBlock = (uint64_t)(offset / blockSize);
    size_t blockOffset = offset - (dataBlock * blockSize);
    uint64_t row = dataBlock / dataCount;
    uint64_t col = dataBlock - (row * dataCount);
    size_t blockRemaining = blockSize - blockOffset;
    uint8_t * byteBuffer = (uint8_t*)buffer;

    return_false_if_msg(offset >= (blockCount*dataCount*blockSize), "Error: param 'offset' out of range: %ld\n", offset);
    return_false_if_msg(size > (blockCount*dataCount*blockSize), "Error: param 'size' out of range: %ld\n", offset);
    return_false_if_msg((offset+size) > (blockCount*dataCount*blockSize), "Error: param 'offset+size' out of range: %ld\n", offset+size);

    return_false_if_msg(!GetRow(row).Verify(), "Error: row '%lx' is corrupt.\n", row);

    uint8_t iv[AES_BLOCK_SIZE];

    while (true)
    {
      size_t toRead = (size>blockRemaining)?blockRemaining:size;
      return_false_if_msg(!__ReadCached(row, col, cryptBuffer.get(), blockSize, 0), "Error: failed to read [%lx,%lx].\n", row, col);
      memset(iv, row, AES_BLOCK_SIZE);
      AES_cbc_encrypt(cryptBuffer.get(), clearBuffer.get(), blockSize, &decryptKey, iv, AES_DECRYPT);
      memcpy(byteBuffer, clearBuffer.get() + blockOffset, toRead);
      byteBuffer += toRead;
      size -= toRead;
      blockRemaining = blockSize;
      blockOffset = 0;

      if (size == 0) { break; }

      if (++col == dataCount)
      {
        col = 0;
        row++;
        return_false_if_msg(!GetRow(row).Verify(), "Error: row '%lx' is corrupt.\n", row);
      }
    }
    return true;
  }

  bool Volume::Read(void * buffer, size_t size, size_t offset)
  {
    if (size == 0) { return true; }

    uint64_t dataBlock = (uint64_t)(offset / blockSize);
    size_t blockOffset = offset - (dataBlock * blockSize);
    uint64_t row = dataBlock / dataCount;
    uint64_t col = dataBlock - (row * dataCount);
    size_t blockRemaining = blockSize - blockOffset;
    uint8_t * byteBuffer = (uint8_t*)buffer;

    return_false_if_msg(offset >= (blockCount*dataCount*blockSize), "Error: param 'offset' out of range: %ld\n", offset);
    return_false_if_msg(size > (blockCount*dataCount*blockSize), "Error: param 'size' out of range: %ld\n", offset);
    return_false_if_msg((offset+size) > (blockCount*dataCount*blockSize), "Error: param 'offset+size' out of range: %ld\n", offset+size);

    return_false_if_msg(!GetRow(row).Verify(), "Error: row '%lx' is corrupt.\n", row);

    while (true)
    {
      size_t toRead = (size>blockRemaining)?blockRemaining:size;
      return_false_if_msg(!__ReadCached(row, col, byteBuffer, toRead, blockOffset), "Error: failed to read [%lx,%lx].\n", row, col);
      byteBuffer += toRead;
      size -= toRead;
      blockRemaining = blockSize;
      blockOffset = 0;
      if (size == 0) { break; }
      if (++col == dataCount)
      {
        col = 0;
        row++;

        return_false_if_msg(!GetRow(row).Verify(), "Error: row '%lx' is corrupt.\n", row);
      }
    }
    return true;
  }


  bool Volume::Delete()
  {
    bool success = true;
    for (auto partition : this->partitions)
    {
      success &= partition->Delete();
    }

    return success;
  }


  bool Volume::__VerifyCell(uint64_t row, uint64_t column)
  {
    return_false_if_msg(column >= partitions.size(), "Error: param 'column' is out of range: %ld >= %ld\n", column, partitions.size());
    Partition * partition = partitions[column];
    return_false_if_msg(partition == NULL, "Error: partition '%ld' is not set\n", column);
    return partition->VerifyBlock(row);
  }

  bool Volume::__WriteCell(uint64_t row, uint64_t column, const void * buffer, size_t size, size_t offset)
  {
    return_false_if_msg(column >= partitions.size(), "Error: param 'column' is out of range: %ld >= %ld\n", column, partitions.size());
    Partition * partition = partitions[column];
    return_false_if_msg(partition == NULL, "Error: partition '%ld' is not set\n", column);
    if (column < dataCount)
    {
      return_false_if(!GetRow(row).Verify());
    }
    return_false_if(!__WriteCached(row, column, buffer, size, offset));
    if (column < dataCount)
    {
      return GetRow(row).Encode();
    }
    return true;
  }

  bool Volume::__ReadCell(uint64_t row, uint64_t column, void * buffer, size_t size, size_t offset)
  {
    return_false_if_msg(column >= partitions.size(), "Error: param 'column' is out of range: %ld >= %ld\n", column, partitions.size());
    Partition * partition = partitions[column];
    return_false_if_msg(partition == NULL, "Error: partition '%ld' is not set\n", column);
    if (!__ReadCached(row, column, buffer, size, offset))
    {
      return_false_if(!GetRow(row).Verify());
      return_false_if_msg(!__ReadCached(row, column, buffer, size, offset), "Error: failed to read block");
    }
    return true;
  }


  bool Volume::__ReadCached(uint64_t row, uint64_t column, void * buffer, size_t size, size_t offset)
  {
    if (cache)
    {
      return cache->Read(row, column, buffer, size, offset);
    }

    return __ReadDirect(row, column, buffer, size, offset);
  }


  bool Volume::__WriteCached(uint64_t row, uint64_t column, const void * buffer, size_t size, size_t offset)
  {
    if (cache)
    {
      return cache->Write(row, column, buffer, size, offset);
    }

    return __WriteDirect(row, column, buffer, size, offset);
  }


  bool Volume::__ReadDirect(uint64_t row, uint64_t column, void * buffer, size_t size, size_t offset)
  {
    return partitions[column]->ReadBlock(row, buffer, size, offset);
  }

  bool Volume::__WriteDirect(uint64_t row, uint64_t column, const void * buffer, size_t size, size_t offset)
  {
    return partitions[column]->WriteBlock(row, buffer, size, offset);
  }

  /*
  bool Volume::GetCellHash(uint64_t blockId, hash_t & hash)
  {
    if (blockId > blockCount) { return false; }
    char fileName[1024];
    snprintf(fileName, sizeof(fileName), "%s-%lx", partitionId.c_str(), blockId);
    uint8_t * buffer = new uint8_t[blockSize];
    memset(buffer, 0, blockSize);
    FILE * file = fopen(fileName, "r");
    fread(buffer, 1, blockSize, file);
    fclose(file);
    return true;
  }
  */
}
