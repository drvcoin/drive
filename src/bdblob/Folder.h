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
#include <string>
#include <map>
#include <memory>
#include <vector>

#include "BlobMetadata.h"
#include "IInputStream.h"
#include "IOutputStream.h"

namespace bdblob
{
  class BlobProvider;

  class Folder
  {
  public:

    enum class EntryType
    {
      FOLDER = 0,
      FILE = 1,
      __MAX__
    };

    struct Entry
    {
      std::string id;
      EntryType type;
      uint32_t createTime;
      uint32_t modifyTime;
      uint64_t size;
      BlobMetadata metadata;
    };

  public:

    static std::unique_ptr<Folder> LoadFolder(BlobProvider * provider, std::string id);

    explicit Folder(BlobProvider * provider, std::string name, std::string id, Folder * parent);

    ~Folder();

    const Entry & Properties() const                        { return this->entries.at("."); }

    const std::map<std::string, Entry> & Entries() const    { return this->entries; }

    void UpdateEntry(std::string name, Entry entry);

    void RemoveEntry(std::string name);

    bool Flush();

    bool IsRootFolder() const;

    std::vector<std::string> Find(std::string pattern) const;

  private:
    
    explicit Folder(BlobProvider * provider);

    std::unique_ptr<Folder> GetParentFolder() const;

    bool Serialize(dfs::IOutputStream & stream);

    bool Deserialize(dfs::IInputStream & stream);

    bool UpdateMetadataCache(bool includeThis = false) const;

    size_t GetSerializedSize() const;

    const Entry * Parent() const;

    static bool SerializeEntry(dfs::IOutputStream & stream, const Entry & entry);

    static bool DeserializeEntry(dfs::IInputStream & stream, Entry & entry);

    static size_t GetEntrySerializedSize(const Entry & entry);

  private:

    std::map<std::string, Entry> entries;

    std::string name;

    bool modified;

    BlobProvider * provider;
  };
}