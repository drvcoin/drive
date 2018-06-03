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

#include <memory>
#include <string>

namespace bdcontract
{
  class Contract
  {
  public:

    const std::string & Name() const        { return this->name; }
    const std::string & Consumer() const    { return this->consumer; }
    const std::string & Provider() const    { return this->provider; }
    uint64_t Size() const                   { return this->size; }

    void SetName(std::string val)           { this->name = std::move(val); }
    void SetConsumer(std::string val)       { this->consumer = std::move(val); }
    void SetProvider(std::string val)       { this->provider = std::move(val); }
    void SetSize(uint64_t val)              { this->size = val; }

    std::string ToString() const;

    static std::unique_ptr<Contract> FromString(const char * str, size_t len);
    static std::unique_ptr<Contract> FromString(const std::string & str);

  private:

    std::string name;

    std::string consumer;

    std::string provider;

    uint64_t size;
  };
}
