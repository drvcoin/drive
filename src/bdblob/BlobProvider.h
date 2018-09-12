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

#include <memory>
#include <string>
#include <memory>
#include <stdint.h>
#include "IBlob.h"
#include "BlobMap.h"

namespace bdblob
{
  class BlobProvider
  {
  public:

    virtual ~BlobProvider() = default;

    virtual std::unique_ptr<IBlob> NewBlob(uint64_t size) = 0;

    virtual std::unique_ptr<IBlob> OpenBlob(std::string id) = 0;

    virtual void DeleteBlob(std::string id) = 0;

  public:

    bool InitializeBlobMap(std::string path)
    {
      this->blobMap = std::make_unique<BlobMap>();
      return this->blobMap->Initialize(std::move(path));
    }

    BlobMap * GetBlobMap() const
    {
      return this->blobMap.get();
    }

  private:

    std::unique_ptr<BlobMap> blobMap = nullptr;
  };
}