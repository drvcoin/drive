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

#include <drive/common/PlatformHelper.h>
#include <drive/common/HttpRequest.h>

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
    auto & relays = this->config->Relays();

#if 0
    if (this->config->ActiveRelay() >= static_cast<int>(relays.size()))
    {
      // Reset relay since all endpoints failed before. We should retry from beginning.
      this->config->ActiveRelay(-1);
      this->config->ActiveRelayEndpoint(-1);
    }
#endif

#ifdef DEBUG_HTTP_RELAY
    printf("HttpRequest::Execute: %s\n", this->url.c_str());
#endif

    int rtn = CURLE_OK;
    bool tryingRelays = this->config->ActiveRelay() < 0;
    if (this->config->ActiveRelay() >= static_cast<int>(relays.size()))
    {
#ifdef DEBUG_HTTP_RELAY
      printf("HttpRequest::ExecuteImpl: activeRelay=%d activeEp=%d\n", this->config->ActiveRelay(), this->config->ActiveRelayEndpoint());
#endif      
      rtn = this->ExecuteImpl();
    }
    else
    {
      while (this->config->ActiveRelay() < static_cast<int>(relays.size()))
      {
#ifdef DEBUG_HTTP_RELAY
        printf("HttpRequest::ExecuteImpl: trying activeRelay=%d activeEp=%d\n", this->config->ActiveRelay(), this->config->ActiveRelayEndpoint());
#endif
        rtn = this->ExecuteImpl();
        if (!tryingRelays)
        {
          break;
        }

        if (rtn != CURLE_COULDNT_RESOLVE_PROXY &&
            rtn != CURLE_COULDNT_RESOLVE_HOST &&
            rtn != CURLE_COULDNT_CONNECT &&
            rtn != CURLE_REMOTE_ACCESS_DENIED &&
            rtn != CURLE_OPERATION_TIMEDOUT &&
            rtn != CURLE_SEND_ERROR)
        {
          // TODO: maybe we should handle more errors like ssl handshake to make sure we were
          // trying to connect to the right server
          break;
        }

        if (this->config->ActiveRelay() < 0 ||
            this->config->ActiveRelayEndpoint() >= static_cast<int>(relays[this->config->ActiveRelay()].endpoints.size()) - 1)
        {
          do
          {
            this->config->ActiveRelay(this->config->ActiveRelay() + 1);
            this->config->ActiveRelayEndpoint(0);
          } while (this->config->ActiveRelay() < static_cast<int>(relays.size()) &&
                  relays[this->config->ActiveRelay()].endpoints.size() == 0);
        }
        else
        {
          this->config->ActiveRelayEndpoint(this->config->ActiveRelayEndpoint() + 1);
        }
      }
    }

#ifdef DEBUG_HTTP_RELAY
    printf("HttpRequest::ExecuteImpl: curl_code=%d activeRelay=%d activeEp=%d\n", rtn, this->config->ActiveRelay(), this->config->ActiveRelayEndpoint());
#endif

    completeCallback(rtn != CURLE_OK);
  }


  static std::string replaceHostInUrl(const std::string & url, const std::string & host)
  {
    size_t start = url.find("://");
    if (start == std::string::npos)
    {
      return url;
    }

    start += sizeof("://") - 1;
    if (start >= url.size())
    {
      return url;
    }

    size_t end = url.find_first_of(":/?", start);
    if (end == start + 1)
    {
      return url;
    }

    std::stringstream ss;
    ss << url.substr(0, start) << host;

    if (end != std::string::npos)
    {
      ss << url.substr(end);
    }

    return ss.str();
  }


  int HttpRequest::ExecuteImpl()
  {
    CURL * curl = curl_easy_init();
    if (curl == NULL)
    {
      completeCallback(true);
      return -1;
    }
#if !(defined(_WIN32) || defined(_WIN64))
  //  curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, HttpClient::CurlOpenSocketCallback);
#endif

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config->ConnectTimeout());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config->RequestTimeout());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, config->UserAgent().c_str());
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    std::string altHost;
    auto & relays = this->config->Relays();

    if (this->config->ActiveRelay() >= 0 &&
        this->config->ActiveRelay() < static_cast<int>(relays.size()) &&
        this->config->ActiveRelayEndpoint() >= 0 &&
        this->config->ActiveRelayEndpoint() < static_cast<int>(relays[this->config->ActiveRelay()].endpoints.size()))
    {
      auto & endpoint = relays[this->config->ActiveRelay()].endpoints[this->config->ActiveRelayEndpoint()];
      if (!endpoint.host.empty() && endpoint.socksPort > 0)
      {
        char relay[BUFSIZ];
        snprintf(relay, sizeof(relay), "socks5h://%s:%u", endpoint.host.c_str(), endpoint.socksPort);
        curl_easy_setopt(curl, CURLOPT_PROXY, relay);

        if (!relays[this->config->ActiveRelay()].name.empty())
        {
          altHost = relays[this->config->ActiveRelay()].name;
        }
      }
    }

    if (altHost.empty())
    {
#ifdef DEBUG_HTTP_RELAY
      printf("Trying to connect to: %s\n", url.c_str());
#endif      
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    }
    else
    {
      std::string altUrl = replaceHostInUrl(url, altHost);
#ifdef DEBUG_HTTP_RELAY
      printf("Trying to connect to: %s\n", altUrl.c_str());
#endif      
      curl_easy_setopt(curl, CURLOPT_URL, altUrl.c_str());
    }

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

    return res;
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
