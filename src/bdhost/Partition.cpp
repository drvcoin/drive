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

#include "Partition.h"
#include "Options.h"

#include <memory.h>
#include <sys/stat.h>

namespace bdhost
{
  Partition::Partition(const char * partitionId, uint64_t blockCount, size_t blockSize) :
    partitionId(partitionId),
    blockCount(blockCount),
    blockSize(blockSize),
    partitionMap(blockCount)
  {
    partitionPath = Options::workDir + this->partitionId;

    mkdir(partitionPath.c_str(), 0777);
    partitionMapFile = partitionPath + "/.partmap";
    FILE * fr = fopen(partitionMapFile.c_str(), "r");
    if (fr != NULL)
    {
      partitionMap.ReadFrom(fr);
      fclose(fr);
    }
  }

  Partition::Partition(std::string & partitionId, uint64_t blockCount, size_t blockSize) :
    Partition(partitionId.c_str(), blockCount, blockSize)
  {
  }

  Partition::~Partition()
  {
  }

  bool Partition::FlushMap()
  {
    FILE * fw = fopen(partitionMapFile.c_str(), "w");
    if (fw != NULL)
    {
      partitionMap.WriteTo(fw);
      fclose(fw);
    }
    return true;
  }

  bool Partition::VerifyBlock(uint64_t index)
  {
    return_false_if_msg(index >= blockCount, "Error: param 'index' is out of bounds: %ld:%ld\n", index, blockCount);
    if (partitionMap[index])
    {
      char fileName[1024];
      snprintf(fileName, sizeof(fileName), "%s/block-%lx", partitionPath.c_str(), index);
      struct stat st;
      return_false_if(stat(fileName, &st) == -1);
      return_false_if((size_t)st.st_size != blockSize);
    }
    return true;
  }

  bool Partition::InitBlock(uint64_t index)
  {
    uint8_t buffer[4096] = {0};
    char fileName[1024];
    snprintf(fileName, sizeof(fileName), "%s/block-%lx", partitionPath.c_str(), index);
    FILE * file = fopen(fileName, "r+");
    if (file == NULL)
    {
      file = fopen(fileName, "w+");
      return_false_if_msg(file == NULL, "Error: failed to open file '%s' for writing.\n", fileName);
    }
    size_t remaining = blockSize;
    while (remaining > 0)
    {
      size_t toWrite = remaining>sizeof(buffer)?sizeof(buffer):remaining;
      fwrite(buffer, toWrite, 1, file);
      remaining -= toWrite;
    }
    fclose(file);

    partitionMap[index] = true;

    FlushMap();

    return true;
  }

  bool Partition::ReadBlock(uint64_t index, void * buffer, size_t size, size_t offset)
  {
    return_false_if_msg(index > blockCount, "Error: 'index' is out of range: %ld >= %ld\n", index, blockCount);
    return_false_if_msg(offset > blockSize, "Error: 'offset' is out of range: %ld > %ld\n", offset, blockSize);
    return_false_if_msg(size > blockSize, "Error: 'size' is out of range: %ld > %ld\n", size, blockSize);
    return_false_if_msg((offset + size) > blockSize, "Error: 'offset+size' is out of range: %ld > %ld\n", (offset + size), blockSize);

    if (partitionMap[index])
    {
      char fileName[1024];
      snprintf(fileName, sizeof(fileName), "%s/block-%lx", partitionPath.c_str(), index);
      FILE * file = fopen(fileName, "r");
      return_false_if_msg(file == NULL, "Error: failed to open file '%s' for reading.\n", fileName);
      fseek(file, offset, SEEK_SET);
      fread(buffer, size, 1, file);
      fclose(file);
    }
    else
    {
      memset(buffer, 0, size);
    }
    return true;
  }

  bool Partition::WriteBlock(uint64_t index, const void * buffer, size_t size, size_t offset)
  {
    return_false_if_msg(index > blockCount, "Error: 'index' is out of range: %ld >= %ld\n", index, blockCount);
    return_false_if_msg(offset > blockSize, "Error: 'offset' is out of range: %ld > %ld\n", offset, blockSize);
    return_false_if_msg(size > blockSize, "Error: 'size' is out of range: %ld > %ld\n", size, blockSize);
    return_false_if_msg((offset + size) > blockSize, "Error: 'offset+size' is out of range: %ld > %ld\n", (offset + size), blockSize);

    if (!partitionMap[index])
    {
      InitBlock(index);
    }

    char fileName[1024];
    snprintf(fileName, sizeof(fileName), "%s/block-%lx", partitionPath.c_str(), index);
    FILE * file = fopen(fileName, "r+");
    if (file == NULL)
    {
      file = fopen(fileName, "w+");
      return_false_if_msg(file == NULL, "Error: failed to open file '%s' for writing.\n", fileName);
    }
    fseek(file, offset, SEEK_SET);
    fwrite(buffer, size, 1, file);
    fclose(file);

    return true;
  }
}
