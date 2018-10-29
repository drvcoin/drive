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
#include "Digest.h"


namespace bdfs
{
  bool Digest::Initialize()
  {
    return SHA256_Init(& this->ctx) != 0;
  }


  bool Digest::Update(const void * data, size_t len)
  {
    return SHA256_Update(& this->ctx, data, len) != 0;
  }


  bool Digest::Finish(sha256_t digest)
  {
    return SHA256_Final(digest, & this->ctx) != 0;
  }


  bool Digest::Compute(const void * data, size_t len, sha256_t digest)
  {
    return SHA256(static_cast<const unsigned char *>(data), len, digest) != nullptr;
  }


  bool Digest::Compare(const void * data, size_t len, sha256_t digest)
  {
    sha256_t target;
    if (!Digest::Compute(data, len, target))
    {
      return false;
    }

    return Digest::Compare(digest, target);
  }


  bool Digest::Compare(sha256_t lhs, sha256_t rhs)
  {
    return memcmp(lhs, rhs, sizeof(sha256_t)) == 0;
  }
}