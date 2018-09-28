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
#include <unistd.h>
#include <fstream>
#include <ctime>
#include "BufferedOutputStream.h"
#include "BufferedInputStream.h"
#include "BlobCache.h"
#include <json/json.h>

namespace dfs
{
  BlobCache::BlobCache(const std::string path, uint64_t expiry_ms)
    : cacheExpiry(expiry_ms)
  {
    if (path.empty())
    {
      return;
    }

    std::ifstream cfg(path.c_str());
    std::string data((std::istreambuf_iterator<char>(cfg)),
                 std::istreambuf_iterator<char>());

    Json::Reader reader;
    Json::Value json;
    if (reader.parse(data.c_str(), data.size(), json, false) && json.isArray())
    {
      for(auto &js : json) {
        CacheValue cv;
        cv.data = js["value"];
        cv.expiry = js["expiry"].asUInt();
        cache[js["key"].asString()] = std::move(cv);
      }
    }

    this->file = fopen(path.c_str(), "w");
  }

  BlobCache::~BlobCache()
  {
    SyncWrite();
    if (this->file)
    {
      fclose(this->file);
      this->file = nullptr;
    }
  }

  void BlobCache::SyncWrite()
  {
    if (!this->file || cache.empty())
    {
      return;
    }

    Json::Value data;
    for(auto &it : cache)
    {
      Json::Value cacheVal;
      cacheVal["key"] = Json::Value(it.first);
      cacheVal["value"] = it.second.data;
      cacheVal["expiry"] = Json::UInt(it.second.expiry);
      data.append(cacheVal);
    }

    auto str = data.toStyledString();
    rewind(this->file);
    fwrite(str.c_str(), 1, str.length(), this->file);
  }

  void BlobCache::SetValue(const std::string &key, const Json::Value &value)
  {
    if (!this->file || key.empty())
    {
      return;
    }

    CacheValue cv;
    cv.data = value;
    cv.expiry = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count() + cacheExpiry;
    this->cache[key] = cv;
  }

  Json::Value BlobCache::GetValue(const std::string &key)
  {
    auto it = cache.find(key);
    if(it != cache.end()) 
    {
      auto &ret = it->second.data;
      if(it->second.expiry > std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count())
      {
        return ret;
      }
      cache.erase(it);
    }
    return Json::Value();
  }

  bool BlobCache::HasKey(const std::string &key) const
  {
    auto it = this->cache.find(key);
    return it != this->cache.end() && it->second.expiry > std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
  }
}
