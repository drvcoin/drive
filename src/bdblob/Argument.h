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
#include <assert.h>
#include <type_traits>

namespace bdblob
{
  class Argument
  {
  public:

    enum class Type
    {
      STRING,
      INTEGER,
      BOOLEAN,
      __MAX__
    };

  public:

    explicit Argument(Type type)
      : type(type)
    {
      assert(type != Type::__MAX__);
    }

    ~Argument()
    {
      if (this->isSet && this->value)
      {
        switch (this->type)
        {
          case Type::STRING:
          {
            delete static_cast<std::string *>(this->value);
            break;
          }

          case Type::INTEGER:
          {
            delete static_cast<int64_t *>(this->value);
            break;
          }

          case Type::BOOLEAN:
          {
            delete static_cast<bool *>(this->value);
            break;
          }

          default:
          {
            assert(false);
            break;
          }
        }

        this->value = nullptr;
      }
    }


    void SetValue(std::string val)
    {
      assert(this->type == Type::STRING);
      if (this->value)
      {
        delete static_cast<std::string *>(this->value);
      }

      this->value = new std::string(std::move(val));
      this->isSet = true;
    }


    void SetValue(const char * val)
    {
      this->SetValue(std::string(val));
    }


    void SetValue(int64_t val)
    {
      assert(this->type == Type::INTEGER);
      if (this->value)
      {
        delete static_cast<int64_t *>(this->value);
      }

      this->value = new int64_t(val);
      this->isSet = true;
    }


    void SetValue(bool val)
    {
      assert(this->type == Type::BOOLEAN);
      if (this->value)
      {
        delete static_cast<bool *>(this->value);
      }

      this->value = new bool(val);
      this->isSet = true;
    }


    template<typename T>
    const T & GetValue() const
    {
      assert(this->isSet);

      bool valid = (this->type == Type::STRING && std::is_convertible<std::string *, T *>::value) ||
                   (this->type == Type::INTEGER && std::is_convertible<int64_t *, T *>::value) ||
                   (this->type == Type::BOOLEAN && std::is_convertible<bool *, T *>::value);

      assert(valid);

      return *((T *)this->value);
    }


    Type GetType() const                    { return this->type; }

    bool IsSet() const                      { return this->isSet; }

    const std::string & Abbrev() const      { return this->abbrev; }

    const std::string & Description() const { return this->description; }

    bool IsRequired() const                 { return this->required; }

    bool IsAnonymouse() const               { return this->anonymouse; }

    void SetAbbrev(std::string val)         { this->abbrev = std::move(val); }

    void SetDescription(std::string val)    { this->description = std::move(val); }

    void SetRequired(bool val)              { this->required = val; }

    void SetAnonymouse(bool val)            { this->anonymouse = val; }

  private:

    Type type;

    void * value = nullptr;

    bool isSet = false;

    std::string abbrev;

    std::string description;

    bool required = false;

    bool anonymouse = false;
  };
}