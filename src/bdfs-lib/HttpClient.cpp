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

#include "HttpClient.h"

#include <cassert>
#include <iostream>

namespace bdfs
{
  HttpClient::HttpClient() :
    connectTimeout(5), executeTimeout(60)
  {
  }

  HttpClient::~HttpClient()
  {
  }

  const char* HttpClient::SecurityToken()
  {
    return GetToken("st");
  }

  const char* HttpClient::Domain()
  {
    return GetToken("domain");
  }

  void HttpClient::SetSecurityToken(const char* securityToken)
  {
    SetToken("st", securityToken);
  }

  void HttpClient::SetAccessToken(const char* accessToken)
  {
    SetToken("at", accessToken);
  }

  void HttpClient::SetDomain(const char* domain)
  {
    SetToken("domain", domain);
  }

  void HttpClient::SetRange(const char* range)
  {
    if(range == NULL)
    {
      this->range = "";
      return;
    }
    this->range = range;
  }

  void HttpClient::SetToken(const char* name, const char* token)
  {
    if (name && token)
    {
      tokens[name] = token;
    }
  }

  const char* HttpClient::GetToken(const char* name)
  {
    if (name)
    {
      std::map<std::string, std::string>::iterator it = tokens.find(name);

      if (it != tokens.end())
      {
        return (it->second).c_str();
      }
    }

    return "";
  }

  char * HttpClient::EncodeStr(const char* str)
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

  void HttpClient::FreeEncodedStr(char * str)
  {
    curl_free(str);
  }

  void HttpClient::SetConnectTimeout(size_t seconds)
  {
    this->connectTimeout = seconds;
  }

  void HttpClient::SetExecTimeout(size_t seconds)
  {
    this->executeTimeout = seconds;
  }

  bool HttpClient::HttpPost(const std::string& url, const Json::Value& input, Json::Value& output, const std::string& language)
  {
    std::stringstream request_stream;
    std::string request_data;
    if (!input.empty())
    {
      if (input.isObject())
      {
        Json::FastWriter writer;
        for (Json::Value::const_iterator it = input.begin(); it != input.end(); it++)
        {
          char * value = EncodeStr(writer.write(*it).c_str());
          if (request_stream.tellp() > 0)
          {
            request_stream << "&";
          }
          request_stream << it.key().asString() << "=" << value;
          curl_free(value);
        }
        request_data = request_stream.str();
      }
      else
      {
        request_data = input.toStyledString();
      }
    }

    std::stringstream body;
    std::stringstream header;
    bool result = HttpGet(url.c_str(), header, body, language.c_str(), request_data);
    if (result)
    {
      if (body.rdbuf()->in_avail() > 0)
      {
        Json::Reader reader;
        if (!reader.parse(body.str(), output))
        {
          result = false;
        }
      }
      else
      {
        output = Json::Value::null;
      }
    }
    return result;
  }

  bool HttpClient::HttpGet(const char* url, std::stringstream& header, std::stringstream& body, const char* language)
  {
    return HttpGet(url, header, body, language, "");
  }


#if !(defined(_WIN32) || defined(_WIN64))

  curl_socket_t HttpClient::CurlOpenSocketCallback(void * clientp, curlsocktype purpose, struct curl_sockaddr * addr)
  {
    return socket(addr->family, addr->socktype | SOCK_CLOEXEC, addr->protocol);
  }

#endif

  bool HttpClient::HttpGet(const char* url, curl_callback headerFunc, curl_callback dataFunc, void* userdata)
  {
    CURL * curl = curl_easy_init();

    if (curl == NULL)
    {
      return false;
    }
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerFunc);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, userdata);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, userdata);
    bool result = HttpGet(curl, url, NULL, "");
    curl_easy_cleanup(curl);
    return result;
  }

  bool HttpClient::HttpGet(const char* url, std::stringstream& header, std::stringstream& body, const char* language, const std::string& input)
  {
    CURL * curl = curl_easy_init();

    if (curl == NULL)
    {
      return false;
    }
    header.rdbuf()->str(std::string());
    header.clear();
    body.rdbuf()->str(std::string());
    body.clear();
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &header);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpClient::WriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

    if(HttpGet(curl, url, language, input))
    {
      std::string line;
      header.seekg(std::ios_base::beg);
      while(header.good())
      {
        getline(header, line);
        size_t pos = line.find("Set-Cookie:");
        if (pos != std::string::npos)
        {
          std::string tokenName;
          size_t start;

          if ((start = line.find("st=", pos)) != std::string::npos)
          {
            tokenName = "st";
          }
          else if ((start = line.find("domain=", pos)) != std::string::npos)
          {
            tokenName = "domain";
          }

          if (start != std::string::npos)
          {
            size_t end = line.find(";", start);
            if (end != std::string::npos)
            {
              std::string value = line.substr(start, end - start);
              if (value != "st=deleted")
              {
                this->tokens[tokenName] = value;
              }
            }
          }
        }
      }
   
      header.clear();
      header.seekg(std::ios_base::beg);
      body.seekg(std::ios_base::beg);
    }
    else
    {
      return false;
    }
    curl_easy_cleanup(curl);
    return true;
  }
    
  bool HttpClient::HttpGet(CURL * curl, const char* url, const char* language, const std::string& input)
  {
#if !(defined(_WIN32) || defined(_WIN64))
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, HttpClient::CurlOpenSocketCallback);
#endif

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectTimeout);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, executeTimeout);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    curl_easy_setopt(curl, CURLOPT_URL, url);

    if(!range.empty())
    {
      curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
    }

    if (!input.empty())
    {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, input.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, input.length());
    }

    std::string cookie;

    if (language)
    {
      cookie += language;
    }

    for (std::map<std::string, std::string>::iterator it = tokens.begin(); it != tokens.end(); it ++)
    {
      if (!it->second.empty())
      {
        cookie += "; ";
        cookie += it->second;
      }
    }

    if (!cookie.empty())
    {
      curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.c_str());
    }

    if (!certPath.empty())
    {
      curl_easy_setopt(curl, CURLOPT_SSLCERT, certPath.c_str());
      if (!keyPath.empty())
      {
        curl_easy_setopt(curl, CURLOPT_SSLKEY, keyPath.c_str());
      }
    }

    if (!caPath.empty())
    {
      curl_easy_setopt(curl, CURLOPT_CAINFO, caPath.c_str());
    }
    else
    {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    }

    bool result = true;
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
      result = false;
    }


    return result;
  }


  bool HttpClient::HttpGet(const char* url, Json::Value& json, const char* language)
  {
    std::stringstream output;
    std::stringstream header;
    bool result = HttpGet(url, header, output, language);
    if (result)
    {
      Json::Reader reader;
      if (!reader.parse(output.str(), json))
      {
        result = false;
      }
    }

    return result;
  }

  size_t HttpClient::WriteToString(void* ptr, size_t size, size_t nmemb, void* str)
  {
    std::stringstream* strStream = (std::stringstream*)str;
    strStream->write((const char*)ptr, size * nmemb);
    return size * nmemb;
  }
}
