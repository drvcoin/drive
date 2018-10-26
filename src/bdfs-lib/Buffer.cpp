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
#include <sstream>
#include <iomanip>
#include "Buffer.h"

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


  std::string Buffer::ToHexString() const
  {
    if (!this->buf || this->size == 0)
    {
      return std::string();
    }

    std::stringstream output;
    output << std::hex;
    for (size_t i = 0; i < this->size; ++i)
    {
      output << std::setw(2) << std::setfill('0') << this->buf[i];
    }

    return output.str();
  }


  static int char2int(char input)
  {
    if (input >= '0' && input <= '9')
    {
      return input - '0';
    }
    else if (input >= 'a' && input <= 'f')
    {
      return input - 'a' + 10;
    }
    else if (input >= 'A' && input <= 'F')
    {
      return input - 'A' + 10;
    }

    return -1;
  }


  bool Buffer::FromHexString(const char * input, size_t len)
  {
    if (!input || len == 0)
    {
      return this->Resize(0);
    }
    else if (len % 2 == 1)
    {
      return false;
    }

    if (!this->Resize(len / 2))
    {
      return false;
    }

    size_t offset = 0;
    for (size_t i = 0; i < this->size; ++i)
    {
      int values[2] = { char2int(input[offset]), char2int(input[offset + 1]) };
      offset += 2;

      if (values[0] < 0 || values[1] < 0)
      {
        return false;
      }

      this->buf[i] = static_cast<uint8_t>(values[0] * 16 + values[1]);
    }

    return true;

  }


  bool Buffer::FromHexString(std::string input)
  {
    return this->FromHexString(input.c_str(), input.size());
  }
}
