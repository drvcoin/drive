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
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>

#include "ErrorCode.h"

namespace bdhost
{
  class HttpContext
  {
  protected:
    HttpContext()
    {
      responseCode = 200;
      responseSent = false;

      responseHeaders["Content-Type"] = "text/html";
    }

    int responseCode;
    std::map<std::string, std::string> responseHeaders;
    bool responseSent;

    static void encodeJsonStr(std::string& str, std::stringstream& ss)
    {
      ss << '"';
      for (size_t i = 0; i < str.length(); i++)
      {
        switch (str[i])
        {
          case '"': ss << "\\\""; break;
          case '\\': ss << "\\\\"; break;
          case '/': ss << "\\/"; break;
          case '\b': ss << "\\b"; break;
          case '\f': ss << "\\f"; break;
          case '\n': ss << "\\n"; break;
          case '\r': ss << "\\r"; break;
          case '\t': ss << "\\t"; break;
          default: ss << str[i];
        }
      }
      ss << '"';
    }

  public:
    virtual ~HttpContext()
    {
    }

    virtual const char* verb(void) = 0;

    virtual const char* path(void) = 0;

    virtual const char* querystring() = 0;

    virtual const char* parameter(const char* name) = 0;

    virtual const char* header(const char* name) = 0;

    virtual const char* cookie(const char* name) = 0;

    virtual std::string dumpParameters() = 0;

    virtual bool isSsl() = 0;

    void setResponseCode(int code)
    {
      responseCode = code;
    }

    void addResponseHeader(const char* name, const char* value)
    {
      responseHeaders[name] = value;
    }

    const char* getResponseHeader(const char* name)
    {
      if (!name)
      {
        return NULL;
      }

      std::map<std::string, std::string>::iterator itr = responseHeaders.find(name);
      if (itr == responseHeaders.end())
      {
        return NULL;
      }

      return itr->second.c_str();
    }

    void writeError(std::string title, std::string details, ErrorCode code)
    {
      writeError(title, details, static_cast<unsigned int>(code));
    }

    void writeError(std::string title, std::string details, unsigned int code)
    {
      std::stringstream ss;
      ss << "{\"Type\":\"Error\",\"Title\":";
      encodeJsonStr(title, ss);
      ss << ",\"Details\":";
      encodeJsonStr(details, ss);
      ss << ",\"Code\":" << code << "}";
      std::string res = ss.str();
      writeResponse(res);
#ifdef DEBUG
      printf("writeError(\"%.*s\", \"%.*s\", %d)\n", (int)title.length(), title.data(), (int)details.length(), details.data(), code);
#endif
    }

    void writeResponse(std::string content)
    {
      writeResponse(content.data(), content.length());
    }

    virtual void writeResponse(const void* data, uint64_t size) = 0;

    virtual void writeHeader() = 0;

    virtual void writeData(const void* data, size_t size) = 0;

    virtual int  readRequest(void* data, unsigned int size) = 0;

    // virtual void sendFile(const char* path, const char* purpose = "inline", const char* name = NULL) = 0;

    virtual void *GetConnection() = 0;

    virtual long remoteIp() const = 0;
  };
}
