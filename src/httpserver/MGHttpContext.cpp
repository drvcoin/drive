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

#include <drive/httpserver/MGHttpContext.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <json/json.h>

namespace bdhttp
{
  using namespace std;

  MGHttpContext::MGHttpContext(struct mg_connection* connection, const struct mg_request_info* request_info, bool owns_connection) :
    _connection(connection),
    _request_info(request_info),
    _verb(request_info->request_method),
    _path(request_info->uri),
    cookies(NULL),
    parameters(NULL),
    owns_connection(owns_connection),
    _body(nullptr),
    _bodylen(0)
  {
  }

  MGHttpContext::~MGHttpContext()
  {
    if (owns_connection)
    {
      free(_connection);
      _connection = NULL;
    }
    delete cookies;
    delete parameters;
    if (_body)
    {
      delete[] _body;
    }
  }

  const char* MGHttpContext::querystring()
  {
    return _request_info->query_string;
  }

  long MGHttpContext::remoteIp() const
  {
    return _request_info->remote_ip;
  }

  void MGHttpContext::ParseParameters(char * buf, size_t len)
  {
    if (!buf || len == 0)
    {
      return;
    }

    char * buffer = new char[len + 1];

    char* last;
    char* c = buf;
    for(;;)
    {
      char * pair = strtok_r(c, "&", &last);
      c = NULL;
      if (!pair) break;

      char * pairLast = NULL;

      const char* name = strtok_r(pair, "= ", &pairLast);
      if (!name) break;
      const char* value = strtok_r(NULL, "&", &pairLast);
      if (!value) continue;
      mg_url_decode(value, strlen(value), buffer, len + 1, 1);

      if (buffer[0] == '"')
      {
        Json::Value json;
        Json::Reader reader;
        if (reader.parse(buffer, strlen(buffer), json, false) && json.isString() && !json.isNull())
        {
          (*parameters)[name] = json.asString();
          continue;
        }
      }

      (*parameters)[name] = buffer;
    }

    delete[] buffer;
  }


  const char* MGHttpContext::parameter(const char* name)
  {
    if (parameters == NULL)
    {
      parameters = new std::map<std::string, std::string>();

      if (_request_info->query_string)
      {
        char * queryString = const_cast<char *>(_request_info->query_string);
        ParseParameters(queryString, strlen(queryString));
      }

      if (_request_info->request_method && 
          strcmp(_request_info->request_method, "POST") == 0 &&
          strcmp(this->header("Content-type"), "application/x-www-form-urlencoded") == 0)
      {
        const char * contentLength = this->header("Content-Length");

        if (contentLength)
        {
          int len = atoi(contentLength);

          char * body = new char[len + 1];

          body[len] = 0;

          int nread = mg_read(_connection, body, len);

          if (nread >= 0)
          {
            body[nread] = 0;

            ParseParameters(body, nread);
          }

          delete[] body;
        }
      }
    }

    if ((parameters != NULL) && (parameters->find(name) != parameters->end()))
      return (*parameters)[name].c_str();
    else
      return "";
  }


  const void * MGHttpContext::body()
  {
    if (!_body && _request_info->request_method &&
        strcmp(_request_info->request_method, "POST") == 0 &&
        strcmp(this->header("Content-Type"), "application/octet-stream") == 0)
    {
      const char * contentLength = this->header("Content-Length");
      if (contentLength)
      {
        int len = atoi(contentLength);
        if (len > 0)
        {
          char * buf = new char[len];
          int nread = mg_read(_connection, buf, len);
          if (nread > 0)
          {
            _bodylen = nread;
            _body = buf;
          }
          else
          {
            delete[] buf;
          }
        }
      }
    }

    return _body;
  }


  size_t MGHttpContext::bodylen()
  {
    if (this->body())
    {
      return _bodylen;
    }

    return 0;
  }

  std::string MGHttpContext::dumpParameters()
  {
    stringstream stream;

    // ensure parameters are built
    this->parameter("");

    for(std::map<std::string, std::string>::const_iterator it = parameters->begin(); it != parameters->end(); it++)
    {
      if (it->first != "st" && it->first.find_first_not_of("_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") == std::string::npos)
      {
        stream << it->first;
        stream << "=\"";
        for (size_t i = 0; i < it->second.length(); i++)
        {
          switch (it->second[i])
          {
            case '"':
            case '$':
              stream << '\\';
              break;
          }
          stream << it->second[i];
        }
        stream << "\"\n";
      }
    }

    return stream.str();
  }


  const char* MGHttpContext::header(const char* name)
  {
    const char* value = mg_get_header(_connection, name);

    return value;
  }

  const char* MGHttpContext::cookie(const char* name)
  {
    if (cookies == NULL)
    {
      cookies = new std::map<std::string, std::string>();
      const char* cookie = mg_get_header(_connection, "Cookie");

      if (cookie == NULL) return "";
      // acting a buffer to write by strtok
      std::string buf = cookie;
      char* last;
      char* c = (char*)buf.c_str();
      for(;;)
      {
        const char* name = strtok_r(c, "= ", &last);
        const char* value = strtok_r(NULL, ";,", &last);
        if (!value) break;

        (*cookies)[name] = value;
        c = NULL;
      }
    }

    if ((cookies != NULL) && (cookies->find(name) != cookies->end()))
      return (*cookies)[name].c_str();
    else
      return "";
  };

  int MGHttpContext::readRequest(void* data, unsigned int size)
  {
    return mg_read(_connection, data, size);
  }

  static const char * GetHttpStatus(int code)
  {
    switch (code)
    {
    case 200:
      return "OK";
    case 206:
      return "Partial Content";
    case 400:
      return "Bad Request";
    case 401:
      return "Unauthorized";
    case 404:
      return "Not Found";
    case 416:
      return "Requested Range Not Satisfiable";
    case 500:
      return "Internal Error";
    default:
      assert(!"Unknown response code. Please add a new case to GetHttpStatus in MGHttpContext.cpp");
      return "Unknown";
    }
  }

  void MGHttpContext::writeResponse(const void* content, uint64_t size)
  {
    mg_printf(_connection, "HTTP/1.1 %d %s\r\n", responseCode, GetHttpStatus(responseCode));

    // output all the headers
    for(std::map<std::string, std::string>::const_iterator it = responseHeaders.begin(); it != responseHeaders.end(); it++)
    {
      mg_printf(_connection, "%s: %s\r\n", it->first.c_str(), it->second.c_str());
    }

    // output content-length
    mg_printf(_connection, "Content-Length: %llu\r\n\r\n", (long long unsigned)size);

    if (content != NULL)
    {
      mg_write(_connection, content, size);
    }
  }

  void MGHttpContext::writeHeader()
  {
    mg_printf(_connection, "HTTP/1.1 %d %s\r\n", responseCode, GetHttpStatus(responseCode));

    // output all the headers
    std::string size;
    std::map<std::string, std::string>::iterator itr = responseHeaders.find("Content-Length");
    if (itr != responseHeaders.end())
    {
      size = itr->second;
      responseHeaders.erase(itr);
    }

    for(std::map<std::string, std::string>::const_iterator it = responseHeaders.begin(); it != responseHeaders.end(); it++)
    {
      mg_printf(_connection, "%s: %s\r\n", it->first.c_str(), it->second.c_str());
    }

    mg_printf(_connection, "Content-Length: %s\r\n\r\n", size.c_str());
  }

  void MGHttpContext::writeData(const void * data, size_t size)
  {
    if (data != NULL)
    {
      mg_write(_connection, data, size);
    }
  }

  void MGHttpContext::sendFile(const char* path)
  {
    mg_send_file(_connection, path);
  }

  bool MGHttpContext::isSsl()
  {
    return _request_info->is_ssl;
  }

  void *MGHttpContext::GetConnection()
  {
    return this->_connection;
  }
}
