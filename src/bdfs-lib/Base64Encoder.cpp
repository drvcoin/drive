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


#include "Base64Encoder.h"

#include <math.h>

namespace bdfs
{
  static const unsigned char base64DecodeMap[256] = {
      255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    //   0 -  15
      255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    //  16 -  31
      255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,    //  32 -  47
       52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,255,255,255,    //  48 -  63
      255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,    //  64 -  79
       15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,    //  80 -  95
      255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,    //  96 - 111
       41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,    // 112 - 127
      255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 128 - 143
      255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 144 - 159
      255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 160 - 175
      255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 176 - 191
      255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 192 - 207
      255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 208 - 223
      255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 224 - 239
      255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255     // 240 - 255
  };

  size_t Base64Encoder::Decode(const char * source, size_t sourceLength, uint8_t * target, size_t targetLength)
  {
    const char * sourceEnd = source + sourceLength;
    uint8_t b1 = 0, b2 = 0, b3 =0 , b4 = 0;
    uint8_t * targetPtr = target;
    while (source < sourceEnd)
    {
      while (source < sourceEnd && (b1 = base64DecodeMap[(unsigned int)*source++]) == 255)
      {
        if (source == sourceEnd)
        {
          return targetPtr - target;
        }
      }
      while (source < sourceEnd && (b2 = base64DecodeMap[(unsigned int)*source++]) == 255)
      {
        if (source == sourceEnd)
        {
          // TODO: this is an error.
          return targetPtr - target;
        }
      }
      while (source < sourceEnd && (b3 = base64DecodeMap[(unsigned int)*source++]) == 255)
      {
        if (source == sourceEnd)
        {
          *targetPtr++ = (b1 << 2) | (b2 >> 4);   // [11111100] | [00000011]
          // TODO: assert (b2 << 4) == 0;
          return targetPtr - target;
        }
      }
      while (source < sourceEnd && (b4 = base64DecodeMap[(unsigned int)*source++]) == 255)
      {
        if (source == sourceEnd)
        {
          *targetPtr++ = (b1 << 2) | (b2 >> 4);   // b1[11111100] | b2[00000011]
          *targetPtr++ = (b2 << 4) | (b3 >> 2);   // b2[11110000] | b3[00001111]
          // TODO: assert (b3 << 6) == 0
          return targetPtr - target;
        }
      }
      *targetPtr++ = (b1 << 2) | (b2 >> 4);           // b1[11111100] | b2[00000011]
      *targetPtr++ = (b2 << 4) | (b3 >> 2);           // b2[11110000] | b3[00001111]
      *targetPtr++ = (b3 << 6) | (b4);                // b3[11000000] | b4[00111111]
    }
    return targetPtr - target;
  }

  const unsigned char base64EncodeMap[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  size_t Base64Encoder::Encode(const uint8_t * source, size_t sourceLength, char * target, size_t targetLength)
  {
    size_t remaining = sourceLength;
    char * targetPtr = target;
    const uint8_t * sourcePtr = source;
    while (true)
    {
      switch (remaining)
      {
      case 0:
        return targetPtr - target;
      case 1:
        *targetPtr++ = base64EncodeMap[sourcePtr[0] >> 2];
        *targetPtr++ = base64EncodeMap[((sourcePtr[0] & 0x3) << 4)];
        *targetPtr++ = '=';
        *targetPtr++ = '=';
        return targetPtr - target;
      case 2:
        *targetPtr++ = base64EncodeMap[sourcePtr[0] >> 2];
        *targetPtr++ = base64EncodeMap[((sourcePtr[0] & 0x3) << 4) | sourcePtr[1] >> 4];
        *targetPtr++ = base64EncodeMap[((sourcePtr[1] & 0xF) << 2)];
        *targetPtr++ = '=';
        return targetPtr - target;
      default:
        *targetPtr++ = base64EncodeMap[sourcePtr[0] >> 2];
        *targetPtr++ = base64EncodeMap[((sourcePtr[0] & 0x3) << 4) | sourcePtr[1] >> 4];
        *targetPtr++ = base64EncodeMap[((sourcePtr[1] & 0xF) << 2) | sourcePtr[2] >> 6];
        *targetPtr++ = base64EncodeMap[sourcePtr[2] & 0x3F];
        sourcePtr += 3;
        remaining -= 3;
        break;
      }
    }
  }
}
