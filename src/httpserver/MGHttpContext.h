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

#include "HttpContext.h"
#include "mongoose.h"
#include <map>

namespace bdhttp
{
  class MGHttpContext : public HttpContext
  {
  private:
    struct mg_connection* _connection;
    const struct mg_request_info* _request_info;
    std::string _verb;
    std::string _path;
    std::map<std::string, std::string>* cookies;
    std::map<std::string, std::string>* parameters;
    bool owns_connection;

    const char * _body;
    size_t _bodylen;

    void ParseParameters(char * buf, size_t len);

  public:
    MGHttpContext(struct mg_connection* connection, const struct mg_request_info* request_info, bool owns_connection);
    ~MGHttpContext();

    virtual const char* verb() { return _verb.c_str(); }
    virtual const char* path() { return _path.c_str(); }
    virtual const char* querystring();
    virtual const char* parameter(const char* name);
    virtual const char* header(const char* name);
    virtual const char* cookie(const char* name);
    virtual const void * body();
    virtual size_t bodylen();

    virtual bool isSsl();
    virtual int readRequest(void* data, unsigned int size);
    virtual void writeResponse(const void* content, uint64_t size);
    virtual void writeHeader();
    virtual void writeData(const void* data, size_t size);
    virtual void sendFile(const char* path);
    
    virtual void *GetConnection();

    virtual std::string dumpParameters();
    virtual long remoteIp() const;
  };
}
