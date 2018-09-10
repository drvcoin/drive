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
#include <map>
#include <memory>
#include <assert.h>
#include <vector>
#include "Argument.h"

namespace bdblob
{
  class ArgumentParser
  {
  public:

    void Register(std::string name, Argument::Type type, bool required, std::string abbrev, std::string description, bool isAnonymouse = false);

    void Register(std::string name, bool required, std::string abbrev, std::string description, std::string defaultValue, bool isAnonymouse = false);
    
    void Register(std::string name, bool required, std::string abbrev, std::string description, const char * defaultValue, bool isAnonymouse = false);
    
    void Register(std::string name, bool required, std::string abbrev, std::string description, int64_t defaultValue, bool isAnonymouse = false);

    void Register(std::string name, bool required, std::string abbrev, std::string description, bool defaultValue, bool isAnonymouse = false);

    void PrintUsage(std::string prefix) const;

    bool Parse(int argc, const char ** argv);

    bool IsSet(std::string name) const;

    template<typename T>
    const T & GetValue(std::string name) const
    {
      auto itr = this->args.find(std::move(name));
      assert(itr != this->args.end() && itr->second != nullptr);

      return itr->second->GetValue<T>();
    }

  private:

    bool ValidateRequired() const;

  private:

    std::map<std::string, std::string> abbrevs;

    std::map<std::string, std::unique_ptr<Argument>> args;

    std::vector<std::string> anonymouse;
  };
}