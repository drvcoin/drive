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

#include <string>
#include <drive/common/Buffer.h>

namespace bdfs
{
  class KeyBase
  {
  public:

    explicit KeyBase(bool isPrivate);

    virtual ~KeyBase() = default;

    KeyBase(const KeyBase & rhs) = delete;

    KeyBase & operator=(const KeyBase & rhs) = delete;

    void SetData(Buffer data);

    void SetData(const uint8_t * data, size_t len);

    bool Save(std::string filename) const;

    bool Load(std::string filename);

    virtual bool Sign(const void * data, size_t len, std::string & output) const;

    virtual bool Verify(const void * data, size_t len, std::string signature) const;

    virtual bool Recover(const void * data, size_t len, std::string signature, std::string sender);

    void Print();

  protected:

    bool IsPrivate() const    { return this->isPrivate; }

    const void * Data() const { return this->data.Buf(); }

    size_t DataLen() const    { return this->data.Size(); }

  private:

    bool isPrivate;

    Buffer data;
  };
}
