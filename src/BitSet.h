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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <memory.h>

class BitSet
{
  class Reference
  {
    static uint64_t trash_bits;
    uint64_t *_ptr;
    size_t _bit;
    
  public:
    Reference(BitSet & bitSet, size_t index);

    Reference & operator = (bool value);
    Reference & operator = (const Reference & other);
    bool operator ~ () const;
    operator bool() const;
    Reference & flip();
  };

  friend class Reference;

private:
  uint64_t * buffer;
  size_t size;

public:
  BitSet(size_t size);
  void Print();
  void ReadFrom(FILE * file);
  void WriteTo(FILE * file);
  Reference operator[](size_t index);
  const bool __Get(size_t index);
  const void __Set(size_t index, bool value);
};
