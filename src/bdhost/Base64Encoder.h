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

#include <stdlib.h>
#include <stdint.h>

namespace bdhost
{
  #define BASE64_ENCODE_SIZE(cb) (size_t)(4 * ((cb + 2) / 3))
  #define BASE64_DECODE_SIZE(cb) (size_t)(3 * ((cb + 3) / 4))

  class Base64Encoder
  {
  public:
      static inline size_t GetEncodedLength(size_t length)
      {
        return BASE64_ENCODE_SIZE(length);
      }

      static inline size_t GetDecodedLength(size_t length)
      {
        return BASE64_DECODE_SIZE(length);
      }

      static size_t Decode(const char * source, size_t sourceLength, uint8_t * target, size_t targetLength);

      static size_t Encode(const uint8_t * source, size_t sourceLength, char * target, size_t targetLength);
  };
}
