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

#include "HttpRequest.h"

#include <sstream>
#include <curl/curl.h>
#include <algorithm>
#include <string.h>

namespace bdfs
{
  HttpRequest::HttpRequest(const char * url, HttpConfig * config) :
    url(url),
    config(config)
  {
    const char * domainStart = strstr(url, "://");
    if (domainStart != NULL)
    {
      domainStart += 3;
      const char * slash = strchrnul(domainStart, '/');
      const char * quest = strchrnul(domainStart, '?');
      const char * hash = strchrnul(domainStart, '#');
      const char * port = strchrnul(domainStart, ':');
      if (slash > quest) { slash = quest; }
      if (slash > hash) { slash = hash; }
      if (slash > port)
      {
        domain.append(domainStart, port - domainStart);
      }
      else
      {
        domain.append(domainStart, slash - domainStart);
      }
      path.append(slash, std::min<const char *>(quest, hash));
    }

    headerCallback = [=](char * ptr, size_t size) {
      if (size > 0)
      {
        const char * colon = strchr(ptr, ':');
        if (colon != NULL)
        {
          std::string name(ptr, colon - ptr);
          while (*(++colon)==' ');
          std::string value(colon, size-(colon-ptr)-2);
          std::transform(name.begin(), name.end(), name.begin(), ::tolower); 
          responseHeaders[name] = value;
          if (name == "set-cookie")
          {
            config->Cookies().Set(value.c_str(), domain, path);
          }
          else
          {
            //printf("%s=%s\n", name.c_str(), value.c_str());
          }
        }
      }
      std::string str(ptr, size);
    };
  }

  HttpRequest::~HttpRequest()
  {
  }

  size_t __HeaderCallback(void * ptr, size_t size, size_t count, void * context)
  {
    size_t realSize = size * count;
    if (realSize > 0)
    {
      ((HttpRequest*)context)->headerCallback((char*)ptr, realSize);
    }
    return realSize;
  }

  size_t __BodyCallback(void * ptr, size_t size, size_t count, void * context)
  {
    size_t realSize = size * count;
    if (realSize > 0)
    {
      ((HttpRequest*)context)->bodyCallback((char*)ptr, realSize);
    }
    return realSize;
  }

  void HttpRequest::Execute()
  {
    CURL * curl = curl_easy_init();
    if (curl == NULL)
    {
      completeCallback(true);
      return;
    }
#if !(defined(_WIN32) || defined(_WIN64))
  //  curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, HttpClient::CurlOpenSocketCallback);
#endif

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config->ConnectTimeout());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config->RequestTimeout());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, config->UserAgent().c_str());
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
/*
    if (!this->proxy.empty())
    {
      curl_easy_setopt(curl, CURLOPT_PROXY, this->proxy.c_str());
    }
*/
    if(!range.empty())
    {
      curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
    }

    struct curl_slist * headers = nullptr;
    if (!this->postdata.empty())
    {
#ifdef DEBUG_HTTP
      printf("POST: %s\n", this->postdata.c_str());
#endif
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, this->postdata.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, this->postdata.length());

      // Disable the Expect header which may cause 1 second delay when trying to post data larger than 1K
      headers = curl_slist_append(headers, "Expect:");
    }

    if (!this->contentType.empty())
    {
      char typeBuf[256];
      sprintf(typeBuf, "Content-Type:%s", this->contentType.c_str());
      headers = curl_slist_append(headers, typeBuf);
    }

    if (headers)
    {
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    std::string cookie;

    /*
    if (language)
    {
      cookie += language;
    }
    */

    cookie += config->Cookies().Format(domain.c_str(), path.c_str());

    if (cookie.size() > 0)
    {
      curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.c_str());
    }

    if (!config->CertPath().empty())
    {
      curl_easy_setopt(curl, CURLOPT_SSLCERT, config->CertPath().c_str());
      if (!config->KeyPath().empty())
      {
        curl_easy_setopt(curl, CURLOPT_SSLKEY, config->KeyPath().c_str());
      }
    }

    if (!config->CaPath().empty())
    {
      curl_easy_setopt(curl, CURLOPT_CAINFO, config->CaPath().c_str());
    }
    else
    {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    }

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, __HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __BodyCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

    CURLcode res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    completeCallback(res != CURLE_OK);
  }

  void HttpRequest::Get(JsonCallback callback)
  {
    std::stringstream * body = new std::stringstream();

    bodyCallback = [=](char* ptr, size_t size){
      body->write(ptr, size);
    };

    completeCallback = [=](bool isError) {
      Json::Value value;
      if (!isError)
      {
        Json::Reader reader;
        std::string contents = body->str();
#ifdef DEBUG_HTTP
        printf("RESPONSE: %s\n", contents.c_str());
#endif
        if (reader.parse(contents.c_str(), contents.size(), value, false))
        {
          isError = value.type() == Json::objectValue &&
            value["Type"].isString() && value["Type"].asString() == "Error";
        }
        else
        {
          isError = true;
        }
      }
      delete body;
      callback(value, isError);
    };
  }

  void HttpRequest::Get(RawCallback callback)
  {
    std::stringstream * body = new std::stringstream();

    bodyCallback = [=](char* ptr, size_t size){
      body->write(ptr, size);
    };

    completeCallback = [=](bool isError) {
      std::string contents;
      if (!isError)
      {
        contents = body->str();
#ifdef DEBUG_HTTP
        printf("RESPONSE: %s\n", contents.c_str());
#endif
      }
      delete body;
      callback(std::move(contents), isError);
    };
  }


  static std::string postImpl(const Json::Value & data)
  {
    std::stringstream rs;
    if (!data.empty())
    {
      if (data.isObject())
      {
        Json::FastWriter writer;
        for (auto it = data.begin(); it != data.end(); ++it)
        {
          char * value = HttpRequest::EncodeStr(writer.write(*it).c_str());
          if (rs.tellp() > 0)
          {
            rs << "&";
          }
          rs << it.key().asString() << "=" << value;
          HttpRequest::FreeEncodedStr(value);
        }

        return rs.str();
      }
      else
      {
        return data.toStyledString();
      }
    }

    return std::string();
  }

  void HttpRequest::Post(const Json::Value & data, JsonCallback callback)
  {
    this->postdata = postImpl(data);
    this->Get(callback);
  }


  void HttpRequest::Post(const Json::Value & data, RawCallback callback)
  {
    this->postdata = postImpl(data);
    this->Get(callback);
  }


  void HttpRequest::Post(const char * type, const void * buf, size_t len, JsonCallback callback)
  {
    if (!buf || len == 0)
    {
      return;
    }

    if (type)
    {
      this->contentType = type;
    }

    this->postdata = std::string(static_cast<const char *>(buf), len);
    this->Get(callback);
  }


  void HttpRequest::Post(const char * type, const void * buf, size_t len, RawCallback callback)
  {
    if (!buf || len == 0)
    {
      return;
    }

    if (type)
    {
      this->contentType = type;
    }

    this->postdata = std::string(static_cast<const char *>(buf), len);
    this->Get(callback);
  }


  char * HttpRequest::EncodeStr(const char* str)
  {
    CURL * curl = curl_easy_init();
    if (curl != NULL)
    {
      char * result = curl_easy_escape(curl, str, 0);
      curl_easy_cleanup(curl);
      return result;
    }
    return NULL;
  }

  void HttpRequest::FreeEncodedStr(char * str)
  {
    curl_free(str);
  }
}
