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


#include <drive/common/HexEncoder.h>


namespace bdfs
{
  static const char * hexEncodeMapUpper =
    "000102030405060708090A0B0C0D0E0F"
    "101112131415161718191A1B1C1D1E1F"
    "202122232425262728292A2B2C2D2E2F"
    "303132333435363738393A3B3C3D3E3F"
    "404142434445464748494A4B4C4D4E4F"
    "505152535455565758595A5B5C5D5E5F"
    "606162636465666768696A6B6C6D6E6F"
    "707172737475767778797A7B7C7D7E7F"
    "808182838485868788898A8B8C8D8E8F"
    "909192939495969798999A9B9C9D9E9F"
    "A0A1A2A3A4A5A6A7A8A9AAABACADAEAF"
    "B0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
    "C0C1C2C3C4C5C6C7C8C9CACBCCCDCECF"
    "D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
    "E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF"
    "F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF"
  ;

  static const char * hexEncodeMapLower =
    "000102030405060708090a0b0c0d0e0f"
    "101112131415161718191a1b1c1d1e1f"
    "202122232425262728292a2b2c2d2e2f"
    "303132333435363738393a3b3c3d3e3f"
    "404142434445464748494a4b4c4d4e4f"
    "505152535455565758595a5b5c5d5e5f"
    "606162636465666768696a6b6c6d6e6f"
    "707172737475767778797a7b7c7d7e7f"
    "808182838485868788898a8b8c8d8e8f"
    "909192939495969798999a9b9c9d9e9f"
    "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
    "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
    "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
    "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
    "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
    "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"
  ;

  static const uint8_t hexDecodeMap[256] = {
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    //   0 -  15
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    //  16 -  31
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    //  32 -  47
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,255,255,255,255,255,255,    //  48 -  63
    255, 10, 11, 12, 13, 14, 15,255,255,255,255,255,255,255,255,255,    //  64 -  79
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    //  80 -  95
    255, 10, 11, 12, 13, 14, 15,255,255,255,255,255,255,255,255,255,    //  96 - 111
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 112 - 127
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 128 - 143
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 144 - 159
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 160 - 175
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 176 - 191
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 192 - 207
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 208 - 223
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,    // 224 - 239
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255     // 240 - 255
  };


  size_t HexEncoder::Encode(const uint8_t * src, size_t srcLen, char * dest, size_t destLen, bool upperCase)
  {
    size_t encodedLen = GetEncodedLength(srcLen);
    if (destLen < encodedLen)
    {
      return 0;
    }

    const uint16_t * hexEncodeMap = reinterpret_cast<const uint16_t *>(upperCase ? hexEncodeMapUpper : hexEncodeMapLower);

    int16_t * ptr = reinterpret_cast<int16_t *>(dest);

    for (size_t i = 0; i < srcLen; ++i)
    {
      * ptr++ = hexEncodeMap[src[i]];
    }

    return encodedLen;
  }


  size_t HexEncoder::Decode(const char * src, size_t srcLen, uint8_t * dest, size_t destLen, bool upperCase)
  {
    size_t decodedLen = GetDecodedLength(srcLen);
    if (destLen < decodedLen)
    {
      return 0;
    }

    const char * ptr = src;
    while (ptr < src + srcLen)
    {
      uint8_t b1 = hexDecodeMap[* ptr++];
      if (b1 == 255)
      {
        return 0;
      }

      uint8_t b2 = hexDecodeMap[* ptr++];
      if (b2 == 255)
      {
        return 0;
      }

      * dest++ = ((b1 << 4) | b2);
    }

    return decodedLen;
  }
}
