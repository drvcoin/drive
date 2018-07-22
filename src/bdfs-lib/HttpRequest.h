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

#include <string>
#include <map>
#include <functional>
#include "json/json.h"

namespace bdfs
{
  typedef std::function<void(Json::Value &, bool)> JsonCallback;
  typedef std::function<void(std::string &&, bool)> RawCallback;

  class HttpRequest
  {
  private:
    std::string url;
    HttpConfig * config;

    std::string domain;
    std::string path;

    std::string contentType;
    std::string postdata;

    std::string range;

    std::map<std::string,std::string> requestHeaders;
    std::map<std::string,std::string> responseHeaders;

    std::string proxy;

  public:
    std::function<void(char*,size_t)> bodyCallback;
    std::function<void(char*,size_t)> headerCallback;
    std::function<void(bool)> completeCallback;

    HttpRequest(const char * url, HttpConfig * config);
    ~HttpRequest();

    void Execute();
    std::string & Url() { return url; }
    std::map<std::string,std::string> & RequestHeaders() { return requestHeaders; }
    std::map<std::string,std::string> & ResponseHeaders() { return responseHeaders; }

    Json::Value & Body();
    void Body(Json::Value & value);

    void Get(JsonCallback callback);
    void Get(RawCallback callback);
    void Post(const Json::Value & data, JsonCallback callback);
    void Post(const Json::Value & data, RawCallback callback);
    void Post(const char * type, const void * buf, size_t len, JsonCallback callback);
    void Post(const char * type, const void * buf, size_t len, RawCallback callback);

    static char * EncodeStr(const char* str);
    static void FreeEncodedStr(char * str);
  };
}
