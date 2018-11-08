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

#include <stdlib.h>
#include <string.h>
#include <drive/common/Buffer.h>

#ifndef BUFSIZ
#define BUFSIZ 8192
#endif

namespace bdfs 
{
  Buffer::~Buffer()
  {
    if (this->buf)
    {
      free(this->buf);
      this->buf = nullptr;
    }
  }


  Buffer::Buffer(Buffer && val)
  {
    if (this->buf)
    {
      free(this->buf);
    }

    this->buf = val.buf;
    this->size = val.size;
    this->memsize = val.memsize;
    val.buf = nullptr;
    val.size = val.memsize = 0;
  }


  Buffer & Buffer::operator=(Buffer && val)
  {
    if (this->buf)
    {
      free(this->buf);
    }

    this->buf = val.buf;
    this->size = val.size;
    this->memsize = val.memsize;
    val.buf = nullptr;
    val.size = val.memsize = 0;
    return *this;
  }


  bool Buffer::Resize(size_t size)
  {
    if (size <= this->memsize)
    {
      this->size = size;
      return true;
    }

    size_t newsize = ((size - 1) / BUFSIZ + 1) * BUFSIZ;

    uint8_t * ptr = static_cast<uint8_t *>(realloc(this->buf, newsize));
    if (ptr)
    {
      this->buf = ptr;
      this->memsize = newsize;
      this->size = size;

      return true;
    }
    else
    {
      return false;
    }
  }
}
