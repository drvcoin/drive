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

#include "HttpConfig.h"
#include "BdObject.h"
#include <string.h>
#include <json/json.h>

namespace bdfs
{
  class HttpRequest;

  class BdSession
  {
  private:
    HttpConfig * config;
    std::string base;
    std::string st;
    std::string __EncodeArgs(BdObject::CArgs & args);

    static SharedMutex mutex;
    static HttpConfig defaultConfig;
    static std::map<std::string, std::shared_ptr<BdSession> > sessions;

    BdSession(const char * base, HttpConfig * config);

    HttpRequest * CreateRequest(std::string & path, const char * method, BdObject::CArgs & args, Json::Value & data);

  public:
    static std::shared_ptr<BdSession> GetSession(const char * base);
    static std::shared_ptr<BdSession> GetSession(const std::string & base);
    static std::shared_ptr<BdSession> CreateSession(const char * base, HttpConfig * config);

    std::string & Base() { return base; }

    bool Call(std::string & path, const char * method, BdObject::CArgs & args, BdObject::Callback callback);
    bool Call(std::string & path, const char * method, BdObject::CArgs & args, BdObject::RawCallback callback);
    bool Call(std::string & path, const char * method, BdObject::CArgs & args, const void * body, size_t bodyLen, BdObject::Callback callback);

    std::shared_ptr<BdObject> CreateObject(const char * name, const char * path, const char * type);
  };
}
