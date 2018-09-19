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

#include <algorithm>
#include <errno.h>
#include <string.h>
#include "RemoteBlob.h"

namespace bdblob
{
  RemoteBlob::RemoteBlob(std::unique_ptr<dfs::Volume> volumePtr, std::string id, uint64_t size)
  {
    this->volume = std::move(volumePtr);

    this->SetId(std::move(id));
    this->SetSize(size);
  }


  RemoteBlob::~RemoteBlob()
  {
    this->Close();
  }


  void RemoteBlob::Close()
  {
    if (this->fp)
    {
      fclose(this->fp);
      this->fp = nullptr;
    }
    volume.reset();
  }


  uint64_t RemoteBlob::Read(uint64_t offset, void * buf, uint64_t len)
  {
    if (!this->volume.get() || !buf || offset > this->Size())
    {
      return 0;
    }

    if(!volume->ReadDecrypt(buf, len, offset))
    {
      len = 0;
    }

     len = std::min<uint64_t>(len, this->Size() - offset);

     return len;
  }


  uint64_t RemoteBlob::Write(uint64_t offset, const void * data, uint64_t len)
  {
    if (!this->volume.get() || !data || offset > this->Size())
    {
      return 0;
    }

    return volume->WriteEncrypt(data, len, offset) ? len : 0;
  }


  bool RemoteBlob::IsOpened() const
  {
    return this->volume.get() != nullptr;
  }
}
