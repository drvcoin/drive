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

#include "BlobMetadata.h"


#define return_false_if(condition) \
    if (condition) { return false; }


namespace bdblob
{
  bool BlobMetadata::Serialize(dfs::IOutputStream & stream) const
  {
    return_false_if(!stream.WriteUInt64(this->size));
    return_false_if(!stream.WriteUInt64(this->blockSize));
    return_false_if(!stream.WriteUInt32(this->partitions.size()));
    for (const auto & partition : this->partitions)
    {
      return_false_if(!stream.WriteString(partition.name));
      return_false_if(!stream.WriteString(partition.provider));
    }
    return true;
  }


  bool BlobMetadata::Deserialize(dfs::IInputStream & stream)
  {
    return_false_if(stream.Remainder() < (sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t)));

    this->size = stream.ReadUInt64();

    this->blockSize = stream.ReadUInt64();

    uint32_t count = stream.ReadUInt32();

    for (uint32_t i = 0; i < count; ++i)
    {
      PartitionInfo info;

      return_false_if(stream.Remainder() < sizeof(uint32_t));
      stream.ReadString(info.name);

      return_false_if(stream.Remainder() < sizeof(uint32_t));
      stream.ReadString(info.provider);

      this->partitions.emplace_back(std::move(info));
    }

    return true;
  }


  size_t BlobMetadata::GetSerializedSize() const
  {
    size_t result = sizeof(this->size) + sizeof(this->blockSize) + sizeof(uint32_t);

    for (const auto & partition : this->partitions)
    {
      result += sizeof(uint32_t) + partition.name.size();
      result += sizeof(uint32_t) + partition.provider.size();
    }

    return result;
  }


  void BlobMetadata::AddPartition(std::string name, std::string provider)
  {
    PartitionInfo info;
    info.name = std::move(name);
    info.provider = std::move(provider);

    this->partitions.emplace_back(std::move(info));
  }
}