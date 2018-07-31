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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <assert.h>
#include <vector>
#include <chrono>
#include <string.h>
#include <inttypes.h>
#include "Volume.h"
#include "Buffer.h"
#include "Cache.h"

namespace dfs
{
  static int mkpath(char* path, mode_t mode)
  {
    char * p = path;

    do
    {
      p = strchr(p + 1, '/');
      if (p)
      {
        *p = '\0';
      }

      int rtn = mkdir(path, mode);

      if (p)
      {
        *p = '/';
      }

      if (rtn == -1 && errno != EEXIST)
      {
        return -1;
      }
    } while (p);

    return 0;
  }


  static void cleanpath(const char * folder)
  {
    std::vector<std::string> names;

    DIR * dir = opendir(folder);
    if (!dir)
    {
      return;
    }

    struct dirent * ent;
    while ((ent = readdir(dir)) != nullptr)
    {
      if (ent->d_type == DT_REG && ent->d_name[0] != '.')
      {
        names.emplace_back(ent->d_name);
      }
    }

    closedir(dir);

    for (const auto & name : names)
    {
      char path[PATH_MAX];
      sprintf(path, "%s/%s", folder, name.c_str());

      unlink(path);
    }
  }


  Cache::Cache(std::string rootPath, Volume * volume, size_t limit, uint32_t flushPolicy)
    : rootPath(std::move(rootPath))
    , limit(limit)
    , flushPolicy(flushPolicy)
    , volume(volume)
    , active(true)
  {
    assert(volume);

    if (rootPath.empty())
    {
      rootPath = "cache";
    }

    if (rootPath[rootPath.size() - 1] == '/')
    {
      rootPath.resize(rootPath.size() - 1);
    }

    mkpath(const_cast<char *>(rootPath.c_str()), 0755);
    cleanpath(rootPath.c_str());

    this->thread = std::thread(std::bind(&Cache::ThreadProc, this));
  }


  Cache::~Cache()
  {
    if (this->active)
    {
      this->active = false;

      {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->hasNotification = true;
        this->cond.notify_all();
      }

      if (this->thread.joinable())
      {
        this->thread.join();
      }
    }

    this->Flush(true);
    cleanpath(this->rootPath.c_str());
  }

  
  bool Cache::Read(uint64_t row, uint64_t column, void * buffer, size_t size, size_t offset)
  {
    assert(buffer);
    if (!this->active ||
        column >= this->volume->DataCount() + this->volume->CodeCount() ||
        size + offset > this->volume->BlockSize())
    {
      return false;
    }

    ReadRequest req{row, column, buffer, size, offset};

    if (!this->requests.Produce(&req))
    {
      return false;
    }

    {
      std::unique_lock<std::mutex> lock(this->mutex);
      this->hasNotification = true;
      this->cond.notify_one();
    }

    if (req.result.Wait())
    {
      return req.result.GetResult();
    }

    return false;
  }


  bool Cache::Write(uint64_t row, uint64_t column, const void * buffer, size_t size, size_t offset)
  {
    assert(buffer);
    if (!this->active ||
        column >= this->volume->DataCount() + this->volume->CodeCount() ||
        size + offset > this->volume->BlockSize())
    {
      return false;
    }

    WriteRequest req{row, column, buffer, size, offset};

    if (!this->requests.Produce(&req))
    {
      return false;
    }

    {
      std::unique_lock<std::mutex> lock(this->mutex);
      this->hasNotification = true;
      this->cond.notify_one();
    }

    if (req.result.Wait())
    {
      return req.result.GetResult();
    }

    return false;
  }


  void Cache::ThreadProc()
  {
    uint64_t ts = static_cast<uint64_t>(time(nullptr));

    while (this->active)
    {
      Request * req = nullptr;

      while (this->requests.Consume(req))
      {
        switch (req->type)
        {
          case RequestType::Read:
          {
            auto read = static_cast<ReadRequest *>(req);
            read->result.Complete(ReadImpl(read->row, read->column, read->buffer, read->size, read->offset));
            break;
          }

          case RequestType::Write:
          {
            auto write = static_cast<WriteRequest *>(req);
            write->result.Complete(WriteImpl(write->row, write->column, write->buffer, write->size, write->offset));
            break;
          }
        }
      }

      uint64_t now = static_cast<uint64_t>(time(nullptr));

      if (now - ts < this->flushPolicy)
      {
        std::unique_lock<std::mutex> lock(this->mutex);
        if (!this->hasNotification)
        {
          this->cond.wait_for(lock, std::chrono::seconds(this->flushPolicy - (now - ts)));
        }
        else
        {
          this->hasNotification = false;
        }
      }
      else
      {
        if (this->Flush(false))
        {
          ts = now;
        }
      }
    }
  }


  bool Cache::ReadImpl(uint64_t row, uint64_t column, void * buffer, size_t size, size_t offset)
  {
    uint8_t * buf = nullptr;
    if (size == this->volume->BlockSize() && offset == 0)
    {
      buf = static_cast<uint8_t *>(buffer);
    }
    else
    {
      buf = new uint8_t[this->volume->BlockSize()];
    }

    char filename[PATH_MAX];
    sprintf(filename, "%s/%" PRIu64 "", this->rootPath.c_str(), row);

    bool success = this->ReadFileBlock(filename, column, buf);

    if (!success)
    {
      success = this->volume->__ReadDirect(row, column, buf, this->volume->BlockSize(), 0);
      if (success)
      {
        // Do not fail if write cache fails since we are just reading data
        this->WriteFileBlock(filename, column, buf);
      }
    }

    if (buf != buffer)
    {
      memcpy(buffer, buf + offset, size);
      delete[] buf;
    }

    this->UpdateTimestamp(row, false);

    return success;
  }


  bool Cache::WriteImpl(uint64_t row, uint64_t column, const void * buffer, size_t size, size_t offset)
  {
    uint8_t * buf = nullptr;
    if (size == this->volume->BlockSize() && offset == 0)
    {
      buf = const_cast<uint8_t *>(static_cast<const uint8_t *>(buffer));
    }
    else
    {
      buf = new uint8_t[this->volume->BlockSize()];
      if (!this->ReadImpl(row, column, buf, this->volume->BlockSize(), 0))
      {
        delete[] buf;
        return false;
      }

      memcpy(buf + offset, buffer, size);
    }

    char filename[PATH_MAX];
    sprintf(filename, "%s/%" PRIu64 "", this->rootPath.c_str(), row);

    bool success = this->WriteFileBlock(filename, column, buf);

    if (buf != buffer)
    {
      delete[] buf;
    }

    if (success)
    {
      this->UpdateTimestamp(row, true);
    }

    return success;
  }


  void Cache::Pop()
  {
    size_t size = this->items.size();

    bool flushed = false;

    auto ts = this->timestamps.begin();

    while (ts != this->timestamps.end() && size == this->items.size())
    {
      auto it = this->items.find(ts->second);
      if (it != this->items.end())
      {
        if (it->second.dirty && !flushed)
        {
          this->Flush(false);
        }

        if (!it->second.dirty)
        {
          char filename[PATH_MAX];
          sprintf(filename, "%s/%" PRIu64 "", this->rootPath.c_str(), ts->second);
          unlink(filename);

          this->items.erase(it);
        }
        else
        {
          ++ts;
          continue;
        }
      }

      this->timestamps.erase(ts++);
    }
  }


  bool Cache::Flush(bool force)
  {
    bool all = true;

    bdfs::Buffer buf;

    uint64_t expire = static_cast<uint64_t>(time(nullptr)) + this->flushPolicy;

    for (auto ts = this->timestamps.begin(); ts != this->timestamps.end(); ++ts)
    {
      if (!force && ts->first > expire)
      {
        // Not expired yet
        break;
      }

      auto itr = this->items.find(ts->second);
      if (itr == this->items.end() || !itr->second.dirty)
      {
        continue;
      }

      char filename[PATH_MAX];
      sprintf(filename, "%s/%" PRIu64 "", this->rootPath.c_str(), itr->first);

      FILE * file = fopen(filename, "rb");
      if (file)
      {
        bool success = true;

        uint64_t column = 0;
        while (fread(&column, 1, sizeof(uint64_t), file) == sizeof(uint64_t))
        {
          buf.Resize(this->volume->BlockSize());

          if (fread(buf.Buf(), 1, this->volume->BlockSize(), file) != this->volume->BlockSize() ||
              !this->volume->__WriteDirect(itr->first, column, buf.Buf(), this->volume->BlockSize(), 0))
          {
            success = false;
            break;
          }
        }

        fclose(file);

        if (success)
        {
          itr->second.dirty = false;
        }
        else
        {
          printf("Error: failed to flush the cache block: row=%llu column=%llu\n", (long long unsigned)itr->first, (long long unsigned)column);
          all = false;
        }
      }

      if (this->requests.Size() > 0)
      {
        // We should respond to pending requests first
        all = (++ts  == this->timestamps.end());
        break;
      }
    }

    return all;
  }


  bool Cache::ReadFileBlock(const char * filename, uint64_t column, void * buffer)
  {
    size_t bytes = 0;

    FILE * file = fopen(filename, "rb");
    if (file)
    {
      uint64_t idx = 0;

      while (fread(&idx, 1, sizeof(uint64_t), file) == sizeof(uint64_t))
      {
        if (idx == column)
        {
          bytes = fread(buffer, 1, this->volume->BlockSize(), file);
          break;
        }
        else
        {
          fseek(file, static_cast<long>(this->volume->BlockSize()), SEEK_CUR);
        }
      }

      fclose(file);
    }

    return bytes == this->volume->BlockSize();
  }


  bool Cache::WriteFileBlock(const char * filename, uint64_t column, const void * buffer)
  {
    size_t bytes = 0;

    FILE * file = fopen(filename, "rb+");
    if (!file)
    {
      file = fopen(filename, "wb+");
    }

    if (file)
    {
      uint64_t idx = 0;

      while (fread(&idx, 1, sizeof(uint64_t), file) == sizeof(uint64_t))
      {
        if (idx == column)
        {
          bytes = fwrite(buffer, 1, this->volume->BlockSize(), file);
          break;
        }
        else
        {
          fseek(file, static_cast<long>(this->volume->BlockSize()), SEEK_CUR);
        }
      }

      if (bytes != this->volume->BlockSize())
      {
        if (fwrite(&column, 1, sizeof(column), file) == sizeof(column))
        {
          bytes = fwrite(buffer, 1, this->volume->BlockSize(), file);
        }
      }

      fclose(file);
    }

    return bytes == this->volume->BlockSize();
  }


  void Cache::UpdateTimestamp(uint64_t row, bool setDirty)
  {
    uint64_t now = static_cast<uint64_t>(time(nullptr));

    auto itr = this->items.find(row);
    if (itr != this->items.end())
    {
      auto range = this->timestamps.equal_range(itr->second.timestamp);
      for (auto it = range.first; it != range.second; ++it)
      {
        if (it->second == row)
        {
          this->timestamps.erase(it);
          break;
        }
      }

      itr->second.timestamp = now;
      if (setDirty)
      {
        itr->second.dirty = true;
      }
    }
    else
    {
      this->items[row] = {
        .timestamp = now,
        .dirty = setDirty
      };
    }

    this->timestamps.emplace(now, row);

    if (this->items.size() > this->limit)
    {
      this->Pop();
    }
  }
}
