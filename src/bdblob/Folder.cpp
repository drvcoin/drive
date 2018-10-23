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
#include "BlobProvider.h"
#include "BlobApi.h"
#include "BufferedOutputStream.h"
#include "BufferedInputStream.h"
#include "BlobMap.h"
#include "Folder.h"

#define MAGIC_HEADER 0x0a0b0c0d

#define return_false_if(condition) \
    if (condition) { return false; }

namespace bdblob
{
  using namespace dfs;
  static const uint64_t FOLDER_BLOB_UNIT_SIZE = 64 * 1024;

  static inline uint32_t now()
  {
    return static_cast<uint32_t>(time(nullptr));
  }


  Folder::Folder(BlobProvider * provider)
    : modified(false)
    , provider(provider)
  {
    assert(provider);
  }


  Folder::Folder(BlobProvider * provider, std::string name, std::string id, Folder * parent)
    : name(std::move(name))
    , modified(true)
    , provider(provider)
  {
    assert(!id.empty());
    assert(provider);

    auto blobMap = provider->GetBlobMap();
    assert(blobMap);

    Entry properties = {};
    properties.type = EntryType::FOLDER;
    properties.id = std::move(id);
    properties.createTime = properties.modifyTime = now();
    properties.size = 0;

    Buffer buffer = blobMap->GetValue(properties.id);
    BufferedInputStream stream{static_cast<const uint8_t *>(buffer.Buf()), buffer.Size()};
    properties.metadata.Deserialize(stream);

    if (parent)
    {
      parent->UpdateEntry(this->name, properties);
      this->entries[".."] = parent->Properties();
      parent->Flush();
    }

    this->entries.emplace(".", std::move(properties));
    this->Flush();
  }


  Folder::~Folder()
  {
    this->Flush();
  }


  std::unique_ptr<Folder> Folder::LoadFolder(BlobProvider * provider, std::string id)
  {
    if (!provider || id.empty())
    {
      return nullptr;
    }

    auto blob = provider->OpenBlob(std::move(id));
    if (!blob || blob->Size() == 0)
    {
      return nullptr;
    }

    bool success = false;
    std::unique_ptr<Folder> folder = std::unique_ptr<Folder>(new Folder(provider));

    uint8_t * buffer = new uint8_t[blob->Size()];
    size_t len = blob->Read(0, buffer, blob->Size());
    if (len > 0)
    {
      BufferedInputStream stream{buffer, len};
      success = folder->Deserialize(stream);
      if (success)
      {
        folder->UpdateMetadataCache();
      }
    }

    delete[] buffer;

    return success ? std::move(folder) : nullptr;
  }


  void Folder::UpdateEntry(std::string name, Folder::Entry entry)
  {
    Entry & properties = this->entries["."];

    if (this->entries.find(name) == this->entries.end())
    {
      properties.size += 1;
      properties.modifyTime = now();

      auto parent = this->GetParentFolder();
      if (parent)
      {
        parent->UpdateEntry(this->name, properties);
      }
    }

    this->entries[name] = std::move(entry);

    this->modified = true;

    this->Flush();

    this->UpdateMetadataCache();
  }


  void Folder::RemoveEntry(std::string name)
  {
    Entry & properties = this->entries["."];

    auto itr = this->entries.find(name);

    if (itr != this->entries.end())
    {
      properties.size -= 1;
      properties.modifyTime = now();

      auto parent = this->GetParentFolder();
      if (parent)
      {
        parent->UpdateEntry(this->name, properties);
      }

      this->entries.erase(itr);
      this->modified = true;
    }

    this->Flush();
  }


  bool Folder::Flush()
  {
    if (!this->modified)
    {
      return true;
    }

    size_t sizeNeeded = this->GetSerializedSize();

    Entry & properties = this->entries["."];

    auto blob = this->provider->OpenBlob(properties.id);

    if (!blob || blob->Size() < sizeNeeded)
    {
      // Blob is not created or it is too small to hold the content. Create a new blob and destroy the old one.
      blob = this->provider->NewBlob(((sizeNeeded - 1) / FOLDER_BLOB_UNIT_SIZE + 1) * FOLDER_BLOB_UNIT_SIZE);
      
      if (blob)
      {
        std::string oldId = std::move(properties.id);
        properties.id = blob->Id();

        auto parent = this->GetParentFolder();
        if (parent)
        {
          parent->UpdateEntry(this->name, properties);
          parent->Flush();
        }

        this->provider->DeleteBlob(oldId);
      }
    }

    return_false_if(!blob);

    BufferedOutputStream stream;
    return_false_if(!this->Serialize(stream));

    return_false_if(blob->Write(0, stream.Buffer(), stream.Offset()) < stream.Offset());

    this->modified = false;

    this->UpdateMetadataCache(true);
    return true;
  }


  bool Folder::Serialize(IOutputStream & stream)
  {
    return_false_if(!stream.WriteUInt32(MAGIC_HEADER));

    return_false_if(!stream.WriteString(this->name));

    return_false_if(!stream.WriteUInt32(static_cast<uint32_t>(this->entries.size())));

    for (const auto & pair : this->entries)
    {
      return_false_if(!stream.WriteString(pair.first));
      return_false_if(!Folder::SerializeEntry(stream, pair.second));
    }

    return true;
  }


  bool Folder::Deserialize(IInputStream & stream)
  {
    return_false_if(stream.Remainder() < sizeof(uint32_t));
    return_false_if(stream.ReadUInt32() != MAGIC_HEADER);

    return_false_if(stream.Remainder() < sizeof(uint32_t));
    stream.ReadString(this->name);

    return_false_if(stream.Remainder() < sizeof(uint32_t));
    uint32_t count = stream.ReadUInt32();

    for (uint32_t i = 0; i < count; ++i)
    {
      std::string key;
      return_false_if(stream.Remainder() < sizeof(uint32_t));
      stream.ReadString(key);

      Entry entry;
      return_false_if(!Folder::DeserializeEntry(stream, entry));

      this->entries.emplace(std::move(key), std::move(entry));
    }

    return true;
  }


  size_t Folder::GetSerializedSize() const
  {
    size_t result = 0;

    result += sizeof(uint32_t); // MAGIC_HEADER
    result += sizeof(uint32_t) + this->name.size();
    result += sizeof(uint32_t); // Entry count

    for (const auto & pair : this->entries)
    {
      result += sizeof(uint32_t) + pair.first.size();
      result += Folder::GetEntrySerializedSize(pair.second);
    }

    return result;
  }


  bool Folder::SerializeEntry(IOutputStream & stream, const Entry & entry)
  {
    return_false_if(!stream.WriteString(entry.id));
    return_false_if(!stream.WriteUInt8(static_cast<uint8_t>(entry.type)));
    return_false_if(!stream.WriteUInt32(entry.createTime));
    return_false_if(!stream.WriteUInt32(entry.modifyTime));
    return_false_if(!stream.WriteUInt64(entry.size));
    return_false_if(!entry.metadata.Serialize(stream));

    return true;
  }


  bool Folder::DeserializeEntry(IInputStream & stream, Entry & entry)
  {
    return_false_if(stream.Remainder() < sizeof(uint32_t));

    stream.ReadString(entry.id);

    return_false_if(stream.Remainder() < sizeof(uint8_t) + sizeof(uint32_t) * 2 + sizeof(uint64_t));

    uint8_t type = stream.ReadUInt8();
    return_false_if(type >= static_cast<uint8_t>(EntryType::__MAX__));
    entry.type = static_cast<EntryType>(type);

    entry.createTime = stream.ReadUInt32();
    entry.modifyTime = stream.ReadUInt32();
    entry.size = stream.ReadUInt64();

    return entry.metadata.Deserialize(stream);
  }


  size_t Folder::GetEntrySerializedSize(const Entry & entry)
  {
    size_t result = 0;
    result += sizeof(uint32_t) + entry.id.size();
    result += sizeof(uint8_t); // type
    result += sizeof(entry.createTime);
    result += sizeof(entry.modifyTime);
    result += sizeof(entry.size);
    result += entry.metadata.GetSerializedSize();
    return result;
  }


  bool Folder::UpdateMetadataCache(bool includeThis) const
  {
    auto blobMap = this->provider->GetBlobMap();
    if (!blobMap)
    {
      return false;
    }

    for (const auto & entry : this->entries)
    {
      if ((entry.first == "." && !includeThis) || entry.first == "..")
      {
        continue;
      }

      blobMap->SetMetadata(entry.second.id, entry.second.metadata);
    }

    return true;
  }


  const Folder::Entry * Folder::Parent() const
  {
    auto itr = this->entries.find("..");

    if (itr == this->entries.end())
    {
      return nullptr;
    }

    return & itr->second;
  }


  bool Folder::IsRootFolder() const
  {
    auto parent = this->Parent();
    auto itr = this->entries.find(".");

    return !parent || itr == this->entries.end() || itr->second.id == parent->id;
  }


  std::unique_ptr<Folder> Folder::GetParentFolder() const
  {
    if (this->IsRootFolder())
    {
      return nullptr;
    }

    auto parentEntry = this->Parent();
    assert(parentEntry);

    return Folder::LoadFolder(this->provider, parentEntry->id);
  }


  std::vector<std::string> Folder::Find(std::string pattern) const
  {
    std::vector<std::string> result;

    for (const auto & pair : this->entries)
    {
      if (pair.first == "." || pair.first == "..")
      {
        continue;
      }

      if (BlobApi::PatternMatch(pattern, pair.first))
      {
        result.emplace_back(pair.first);
      }
    }

    return std::move(result);
  }
}