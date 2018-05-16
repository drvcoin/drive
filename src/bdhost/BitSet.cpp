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

#include "BitSet.h"
#include "Util.h"

namespace bdhost
{
  uint64_t BitSet::Reference::trash_bits = 0;

  BitSet::Reference::Reference(BitSet & bitSet, size_t index)
  {
    if (index < bitSet.size)
    {
      _ptr = & bitSet.buffer[index >> 6];
    }
    else
    {
      _ptr = & trash_bits;
    }
    _bit = (uint8_t)(index & 0x3F);
  }

  BitSet::Reference & BitSet::Reference::operator = (bool value)
  {
    if (value)
    {
      *_ptr |= ((uint64_t)0x1<<(63-_bit));
    }
    else
    {
      *_ptr &= ~((uint64_t)0x1<<(63-_bit));
    }
    return *this;
  }

  BitSet::Reference & BitSet::Reference::operator = (const BitSet::Reference & other)
  {
    if (*(other._ptr) & (0x1<<(63-other._bit)))
    {
      *_ptr |= ((uint64_t)0x1<<(63-_bit));
    }
    else
    {
      *_ptr &= ~((uint64_t)0x1<<(63-_bit));
    }
    return *this;
  }

  bool BitSet::Reference::operator ~ () const
  {
    return (*(_ptr) & ((uint64_t)0x1<<(63-_bit))) == 0;
  }

  BitSet::Reference::operator bool() const
  {
    return (*(_ptr) & ((uint64_t)0x1<<(63-_bit))) != 0;
  }

  BitSet::Reference & BitSet::Reference::flip()
  {
    *(_ptr) ^= ((uint64_t)0x1<<(63-_bit));
    return *this;
  }

  BitSet::BitSet(size_t size) :
    size(size)
  {
    size_t bufferSize = (size & 0x3F)?(size>>6)+1:(size>>6);
    buffer = new uint64_t[bufferSize];
    memset(buffer, 0, sizeof(uint64_t)*bufferSize);
  }

  void BitSet::Print()
  {
    for (size_t i = 0; i < size; i++)
    {
      printf("%u", __Get(i));
    }
    puts("");
  }

  void BitSet::ReadFrom(FILE * file)
  {
    size_t words = size >> 6;
    size_t i = 0;
    uint64_t netBytes;
    for (; i < words; i++)
    {
      fread(&netBytes, sizeof(uint64_t), 1, file);
      buffer[i] = ntohll(netBytes);
    }
    uint64_t remBits = (0x3F & size);
    if (remBits)
    {
      uint64_t remBytes = (0x7&remBits)?(remBits>>3)+1:(remBits>>3);
      netBytes = 0;
      fread(&netBytes, remBytes, 1, file);
      buffer[i] = ntohll(netBytes);
    }
  }

  void BitSet::WriteTo(FILE * file)
  {
    size_t words = size >> 6;
    size_t i = 0;
    for (; i < words; i++)
    {
      uint64_t netBytes = htonll(buffer[i]);
      fwrite(&netBytes, sizeof(uint64_t), 1, file);
    }
    uint64_t remBits = (0x3F & size);
    if (remBits)
    {
      uint64_t remBytes = (0x7&remBits)?(remBits>>3)+1:(remBits>>3);
      uint64_t netBytes = htonll(buffer[i]);
      fwrite(&netBytes, remBytes, 1, file);
    }
  }

  BitSet::Reference BitSet::operator[](size_t index)
  {
    return BitSet::Reference(*this, index);
  }

  const bool BitSet::__Get(size_t index)
  {
    if (index >= size) { return false; }
    uint8_t bit = (uint8_t)(index & 0x3F);
    size_t word = index >> 6;
    return (buffer[word] & ((uint64_t)0x1<<(63-bit))) != 0;
  }

  const void BitSet::__Set(size_t index, bool value)
  {
    if (index >= size) { return; }
    uint8_t bit = (uint8_t)(index & 0x3F);
    size_t word = index >> 6;
    if (value)
    {
      buffer[word] |= ((uint64_t)0x1<<(63-bit));
    }
    else
    {
      buffer[word] &= ~((uint64_t)0x1<<(63-bit));
    }
  }
}
