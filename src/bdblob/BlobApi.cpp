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
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sstream>
#include "BlobConfig.h"
#include "Handle.h"
#include "BlobApi.h"


namespace bdblob
{
  static inline uint32_t now()
  {
    return static_cast<uint32_t>(time(nullptr));
  }


  static void closeFile(FILE * file)
  {
    if (file)
    {
      fclose(file);
    }
  }


  bool BlobApi::Initialize(std::unique_ptr<BlobProvider> provider, std::string rootId)
  {
    assert(!this->provider && !this->root);
    assert(provider);

    this->ClearError();

    this->provider = std::move(provider);

    if (!rootId.empty())
    {
      this->root = Folder::LoadFolder(this->provider.get(), rootId);
      if (!this->root)
      {
        this->error = BlobError::NOT_FOUND;
        return false;
      }
    }
    else
    {
      auto blob = this->provider->NewBlob(BlobConfig::BlockSize());
      if (!blob)
      {
        this->error = BlobError::IO_ERROR;
        return false;
      }

      this->root = std::make_shared<Folder>(this->provider.get(), std::string(), blob->Id(), nullptr);

      BlobConfig::SetRootId(blob->Id(), this->provider.get());
    }

    return true;
  }


  std::map<std::string, Folder::Entry> BlobApi::List(std::string path)
  {
    this->ClearError();

    std::map<std::string, Folder::Entry> result;

    auto parts = SplitPath(std::move(path));

    std::shared_ptr<Folder> folder;
    if (parts.empty())
    {
      folder = this->root;
    }
    else
    {
      folder = this->GetParentFolder(parts);
    }

    if (!folder)
    {
      this->error = BlobError::NOT_FOUND;
      return result;
    }

    if (!parts.empty())
    {
      const auto & entries = folder->Entries();
      auto itr = entries.find(parts[parts.size() - 1]);
      if (itr == entries.end())
      {
        this->error = BlobError::NOT_FOUND;
        return result;
      }

      if (itr->second.type == Folder::EntryType::FILE)
      {
        result.emplace(itr->first, itr->second);
        return result;
      }

      folder = Folder::LoadFolder(this->provider.get(), itr->second.id);
      if (!folder)
      {
        this->error = BlobError::IO_ERROR;
        return result;
      }
    }

    return folder->Entries();
  }


  bool BlobApi::Mkdir(std::string path)
  {
    this->ClearError();

    if (path.find_first_of("*?") != std::string::npos)
    {
      this->error = BlobError::INVALID_NAME;
      return false;
    }

    auto parts = SplitPath(std::move(path));
    if (parts.empty())
    {
      this->error = BlobError::ALREADY_EXIST;
      return false;
    }

    auto folder = this->GetParentFolder(parts);
    if (!folder)
    {
      this->error = BlobError::NOT_FOUND;
      return false;
    }

    const auto & entries = folder->Entries();
    auto itr = entries.find(parts[parts.size() - 1]);
    if (itr != entries.end())
    {
      this->error = BlobError::ALREADY_EXIST;
      return false;
    }

    auto blob = this->provider->NewBlob(BlobConfig::BlockSize());
    if (!blob)
    {
      this->error = BlobError::IO_ERROR;
      return false;
    }

    auto target = std::make_unique<Folder>(this->provider.get(), parts[parts.size() - 1], blob->Id(), folder.get());
    target->Flush();

    if (BlobConfig::RootId() != this->RootId())
    {
      BlobConfig::SetRootId(this->RootId(), this->provider.get());
    }

    return true;
  }


  bool BlobApi::ParsePutTarget(const std::string & path, const std::string & localPath, std::shared_ptr<Folder> & folder, std::string & filename)
  {
    auto parts = SplitPath(path);

    if (parts.empty())
    {
      folder = this->root;
      filename = BaseName(localPath);
    }
    else
    {
      folder = this->GetParentFolder(parts);
      filename = parts[parts.size() - 1];

      if (!folder)
      {
        this->error = BlobError::NOT_FOUND;
        return false;
      }

      const auto & entries = folder->Entries();
      auto itr = entries.find(filename);
      if (itr != entries.end())
      {
        if (itr->second.type == Folder::EntryType::FILE)
        {
          this->error = BlobError::ALREADY_EXIST;
          return false;
        }

        folder = Folder::LoadFolder(this->provider.get(), itr->second.id);
        if (!folder)
        {
          this->error = BlobError::IO_ERROR;
          return false;
        }

        filename = BaseName(localPath);
      }
    }

    return true;
  }


  bool BlobApi::Put(std::string path, std::string localPath, bool force)
  {
    this->ClearError();

    if (path.find_first_of("*?") != std::string::npos)
    {
      this->error = BlobError::INVALID_NAME;
      return false;
    }

    // Step 1: Get parent folder and basename
    std::shared_ptr<Folder> folder;
    std::string filename;
    std::string oldBlobId;
    uint32_t createTime = 0;
    if (!this->ParsePutTarget(path, localPath, folder, filename))
    {
      if (this->error == BlobError::ALREADY_EXIST && force)
      {
        const auto & entry = folder->Entries().at(filename);
        createTime = entry.createTime;
        oldBlobId = entry.id;
        this->ClearError();
      }
      else
      {
        return false;
      }
    }

    // Step 2: Get local file properties
    Handle<FILE *> file{ fopen(localPath.c_str(), "rb"), closeFile };
    if (!file.Value())
    {
      return false;
    }

    fseeko(file.Value(), 0, SEEK_END);
    uint64_t fileSize = ftello(file.Value());
    rewind(file.Value());

    // Step 3: Create a new blob (not used yet)
    uint64_t blobSize = fileSize > 0 ? (((fileSize - 1) / BlobConfig::BlockSize()) + 1) * BlobConfig::BlockSize() : BlobConfig::BlockSize();

    auto blob = this->provider->NewBlob(blobSize);
    if (!blob)
    {
      this->error = BlobError::IO_ERROR;
      return false;
    }

    // Step 4: Update the folder entry to point to the new blob, and delete the old blob if existed.
    Folder::Entry entry;
    entry.id = blob->Id();
    entry.type = Folder::EntryType::FILE;
    entry.createTime = entry.modifyTime = now();
    entry.size = fileSize;
    if (createTime > 0)
    {
      entry.createTime = createTime;
    }

    folder->UpdateEntry(filename, std::move(entry));

    if (BlobConfig::RootId() != this->RootId())
    {
      BlobConfig::SetRootId(this->RootId(), this->provider.get());
    }

    if (!oldBlobId.empty())
    {
      this->provider->DeleteBlob(std::move(oldBlobId));
    }

    // Step 5: Put the actual data to the new blob
    std::unique_ptr<uint8_t[]> buffer = std::unique_ptr<uint8_t[]>(new uint8_t[BlobConfig::BlockSize()]);
    off_t offset = 0;
    size_t bytes = 0;
    uint64_t totalBytes = 0;
    while ((bytes = fread(buffer.get() + offset, 1, BlobConfig::BlockSize(), file.Value())) > 0)
    {
      if (blob->Write(offset, buffer.get() + offset, bytes) < bytes)
      {
        break;
      }

      offset += bytes;
      totalBytes += bytes;
    }

    if (totalBytes < fileSize)
    {
      this->error = BlobError::IO_ERROR;
      return false;
    }

    return true;
  }


  bool BlobApi::Get(std::string path, std::string localPath, bool force)
  {
    this->ClearError();

    if (!force)
    {
      struct stat stats;
      if (!(stat(localPath.c_str(), &stats) == -1 && errno == ENOENT))
      {
        this->error = BlobError::ALREADY_EXIST;
        return false;
      }
    }

    auto entry = this->Info(path);
    if (!entry)
    {
      return false;
    }

    if (entry->type != Folder::EntryType::FILE)
    {
      this->error = BlobError::NOT_SUPPORTED;
      return false;
    }

    auto blob = this->provider->OpenBlob(entry->id);
    if (!blob)
    {
      this->error = BlobError::IO_ERROR;
      return false;
    }

    Handle<FILE *> file{ fopen(localPath.c_str(), "wb"), closeFile };
    if (!file.Value())
    {
      this->error = BlobError::IO_ERROR;
      return false;
    }

    std::unique_ptr<uint8_t[]> buffer{new uint8_t[BlobConfig::BlockSize()]};
    off_t offset = 0;
    uint64_t bytes = 0;
    uint64_t totalBytes = 0;
    uint64_t remaining = entry->size;

    while (remaining > 0 && (bytes = blob->Read(offset, buffer.get(), BlobConfig::BlockSize())) > 0)
    {
      bytes = std::min<uint64_t>(bytes, remaining);
      if (fwrite(buffer.get(), 1, bytes, file.Value()) < bytes)
      {
        break;
      }

      offset += bytes;
      totalBytes += bytes;
      remaining -= bytes;
    }

    if (totalBytes < entry->size)
    {
      this->error = BlobError::IO_ERROR;
      return false;
    }

    return true;
  }


  bool BlobApi::Delete(std::string path)
  {
    this->ClearError();

    auto parts = SplitPath(std::move(path));
    if (parts.empty())
    {
      this->error = BlobError::NOT_SUPPORTED;
      return false;
    }

    auto folder = this->GetParentFolder(parts);
    const auto & entries = folder->Entries();
    auto itr = entries.find(parts[parts.size() - 1]);
    if (itr == entries.end())
    {
      return true;
    }

    if (itr->second.type == Folder::EntryType::FOLDER)
    {
      auto target = Folder::LoadFolder(this->provider.get(), itr->second.id);
      if (target && target->Entries().size() > 2)
      {
        this->error = BlobError::FOLDER_NOT_EMPTY;
        return false;
      }
    }

    this->provider->DeleteBlob(itr->second.id);
    folder->RemoveEntry(parts[parts.size() - 1]);

    return true;
  }


  std::unique_ptr<Folder::Entry> BlobApi::Info(std::string path)
  {
    this->ClearError();

    std::shared_ptr<Folder> folder;
    std::string filename;

    auto parts = SplitPath(path);

    if (parts.empty())
    {
      folder = this->root;
      filename = ".";
    }
    else
    {
      folder = this->GetParentFolder(parts);
      filename = parts[parts.size() - 1];
    }

    if (!folder)
    {
      this->error = BlobError::NOT_FOUND;
      return nullptr;
    }

    const auto & entries = folder->Entries();
    auto itr = entries.find(filename);
    if (itr == entries.end())
    {
      this->error = BlobError::NOT_FOUND;
      return nullptr;
    }

    return std::make_unique<Folder::Entry>(itr->second);
  }


  std::vector<std::string> BlobApi::SplitPath(std::string path)
  {
    std::vector<std::string> result;

    if (path.empty())
    {
      return result;
    }

    size_t offset = (path[0] == '/' || path[0] == '\\') ? 1 : 0;

    std::string delim = "/\\";

    while (offset < path.size())
    {
      size_t end = path.find_first_of(delim, offset);
      if (end == std::string::npos)
      {
        end = path.size();
      }

      if (end > offset)
      {
        std::string name = path.substr(offset, end - offset);
        if (name == ".." && result.size() > 0)
        {
          result.pop_back();
        }
        else if (name != ".")
        {
          result.emplace_back(std::move(name));
        }
      }

      offset = end + 1;
    }

    return std::move(result);
  }


  std::string BlobApi::JoinPaths(const std::vector<std::string> & parts, size_t offset, size_t len)
  {
    std::stringstream ss;

    for (size_t i = offset; i < std::min<size_t>(len, parts.size()); ++i)
    {
      ss << "/" << parts[i]; 
    }

    return ss.str();
  }


  std::string BlobApi::BaseName(std::string path)
  {
    std::string delim = "/\\";

    size_t offset = path.find_last_of(delim);
    if (offset == path.size() - 1)
    {
      return std::string();
    }
    else if (offset != std::string::npos)
    {
      return path.substr(offset + 1);
    }
    else
    {
      return path;
    }
  }


  std::shared_ptr<Folder> BlobApi::GetParentFolder(const std::vector<std::string> & parts)
  {
    if (parts.empty())
    {
      return nullptr;
    }

    if (parts.size() == 1)
    {
      return this->root;
    }

    std::shared_ptr<Folder> parent = this->root;
    for (size_t i = 0; i < parts.size() - 1; ++i)
    {
      const auto & entries = parent->Entries();

      auto itr = entries.find(parts[i]);
      if (itr == entries.end())
      {
        return nullptr;
      }

      parent = Folder::LoadFolder(this->provider.get(), itr->second.id);
      if (!parent)
      {
        return nullptr;
      }
    }

    return parent;
  }


  static bool matchPattern(char const * needle, char const * haystack)
  {
    for (; *needle != '\0'; ++needle)
    {
      switch (*needle)
      {
        case '?':
        {
          if (*haystack == '\0')
          {
            return false;
          }
          ++haystack;
          break;
        }

        case '*':
        {
          if (needle[1] == '\0')
          {
            return true;
          }
          size_t max = strlen(haystack);
          for (size_t i = 0; i < max; i++)
          {
            if (matchPattern(needle + 1, haystack + i))
            {
              return true;
            }
          }
          return false;
        }

        default:
        {
          if (*haystack != *needle)
          {
            return false;
          }
          ++haystack;
          break;
        }
      }
    }

    return *haystack == '\0';
  }

  bool BlobApi::PatternMatch(std::string pattern, std::string target)
  {
    return matchPattern(pattern.c_str(), target.c_str());
  }
}
