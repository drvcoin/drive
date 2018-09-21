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

    template<typename T>
    void Register(std::string name, bool required, char abbrev, std::string description, bool isAnonymouse, T defaultValue)
    {
      assert(!name.empty());
      assert(this->args.find(name) == this->args.end());

      auto ptr = new Argument<T>();
      ptr->SetRequired(required);
      ptr->SetDescription(description);
      ptr->SetDefaultValue(std::move(defaultValue));
      ptr->SetAnonymouse(isAnonymouse);

      if (abbrev != 0)
      {
        assert(this->abbrevs.find(abbrev) == this->abbrevs.end());
        this->abbrevs[abbrev] = name;
        ptr->SetAbbrev(abbrev);
      }

      if (isAnonymouse)
      {
        this->anonymouse.emplace_back(name);
      }

      this->args[std::move(name)] = std::unique_ptr<ArgumentBase>(ptr);
    }

    void PrintUsage(std::string prefix) const;

    bool Parse(int argc, const char ** argv);

    bool IsSet(std::string name) const;

    ArgumentBase * GetArgument(std::string name) const;

    ArgumentBase & operator[](std::string name) const;

  private:

    bool ValidateRequired() const;

    bool Parse(int argc, const char ** argv, std::map<std::string, std::string> & params) const;

  private:

    std::map<char, std::string> abbrevs;

    std::map<std::string, std::unique_ptr<ArgumentBase>> args;

    std::vector<std::string> anonymouse;
  };
}