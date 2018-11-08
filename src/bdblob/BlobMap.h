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
#include <map>
#include <string>
#include "BlobMetadata.h"
#include <drive/common/Buffer.h>

namespace bdblob
{
  class BlobMap
  {
  private:

    struct Index
    {
      off_t offset;
      size_t size;
    };

  public:

    ~BlobMap();

    bool Initialize(std::string path);

    void SetValue(std::string key, const void * data, size_t len);

    bdfs::Buffer GetValue(std::string key) const;

    void SetMetadata(std::string key, const BlobMetadata & metadata);

    bool GetMetadata(std::string key, BlobMetadata & metadata) const;

    bool HasKey(std::string key) const;

  private:

    std::map<std::string, Index> indexes;

    FILE * file = nullptr;

    std::string filename;
  };
}
