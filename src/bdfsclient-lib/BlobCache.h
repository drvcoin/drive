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

#include <stdio.h>
#include <unordered_map>
#include <string>
#include <chrono>
#include "json/value.h"
#include "Buffer.h"

namespace dfs
{
  typedef std::chrono::steady_clock Clock;

  // Persistent Lookup Cache with Expiry time
  class BlobCache
  {
    struct CacheValue
    {
      Json::Value data;
      uint64_t expiry;
    };

  public:

    explicit BlobCache(const std::string path = "lookup_cache", uint64_t expiry_ms = 5*60*1000);

    ~BlobCache();

    void SetValue(const std::string &key, const Json::Value &value);

    bdfs::Buffer GetValueBuffer(const std::string &key) const;

    Json::Value GetValue(const std::string &key);

    bool HasKey(const std::string &key) const;

  private:

    void SyncWrite();

    std::unordered_map<std::string, CacheValue> cache;

    FILE * file = nullptr;

    uint64_t cacheExpiry; // In milliseconds
  };
}
