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
#include <functional>
#include "json/json.h"

namespace bdfs
{
  class BdObject
  {
  public:
    typedef std::map<std::string, Json::Value> CArgs;
    typedef std::function<void(Json::Value&, bool)> Callback;
    typedef std::function<void(std::string &&, bool)> RawCallback;

  private:
    std::string base;
    std::string path;
    std::string name;
    std::string type;

  protected:

    const std::string & Base() const  { return base; }

  public:
    BdObject(const char * base, const char * name, const char * path, const char * type);

    virtual ~BdObject() = default;

    std::string & Name() { return name; }
    std::string & Path() { return path; }
    std::string & Type() { return type; }

    bool Call(const char * method, CArgs & args, Callback callback);
    bool Call(const char * method, CArgs & args, RawCallback callback);
    bool Call(const char * method, CArgs & args, const void * body, size_t len, Callback callback);
  };
}
