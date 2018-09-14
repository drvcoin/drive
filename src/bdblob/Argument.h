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
#include <stdint.h>
#include <typeinfo>
#include <assert.h>

namespace bdblob
{
  enum class ArgumentType
  {
    STRING,
    INTEGER,
    BOOLEAN
  };


  class ArgumentBase
  {
  public:

    virtual ~ArgumentBase() = default;

    bool IsString() const                   { return this->type == ArgumentType::STRING; }
    bool IsIntegral() const                 { return this->type == ArgumentType::INTEGER; }
    bool IsBoolean() const                  { return this->type == ArgumentType::BOOLEAN; }

    ArgumentType Type() const               { return this->type; }

    virtual const std::string & AsString() const    { assert(false); }
    virtual int64_t AsInt() const                   { assert(false); }
    virtual bool AsBoolean() const                  { assert(false); }

    bool IsSet() const                      { return this->isSet; }
    char Abbrev() const                     { return this->abbrev; }
    const std::string & Description() const { return this->description; }
    bool IsRequired() const                 { return this->required; }
    bool IsAnonymouse() const               { return this->anonymouse; }

    void SetAbbrev(char val)                { this->abbrev = val; }
    void SetDescription(std::string val)    { this->description = std::move(val); }
    void SetRequired(bool val)              { this->required = val; }
    void SetAnonymouse(bool val)            { this->anonymouse = val; }

  protected:

    explicit ArgumentBase(ArgumentType type)
      : type(type)
    {
    }

    void SetIsSet(bool val)                 { this->isSet = val; }

  private:

    ArgumentType type;
    bool isSet = false;
    bool required = false;
    bool anonymouse = false;
    char abbrev = 0;
    std::string description;
  };


  template<typename T>
  class Argument : public ArgumentBase
  {
  public:

    Argument();

    const std::string & AsString() const override     { assert(false); }
    int64_t AsInt() const override                    { assert(false); }
    bool AsBoolean() const override                   { assert(false); }

    template<typename V>
    void SetValue(V && val)
    {
      this->value = std::forward<V>(val);
      this->SetIsSet(true);
    }

    template<typename V>
    void SetDefaultValue(V && val)
    {
      this->value = std::forward<V>(val);
    }

  private:

    T value = T();
  };


  using StringArgument = Argument<std::string>;
  using IntArgument = Argument<int64_t>;
  using BoolArgument = Argument<bool>;
}