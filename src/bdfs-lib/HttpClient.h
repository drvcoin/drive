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

#include <curl/curl.h>
#include "json/json.h"

#include <sstream>
#include <map>

namespace bdfs
{
  typedef size_t (*curl_callback) (char*, size_t, size_t, void*); 

  class HttpClient
  {
  private:
    std::string certPath, keyPath, caPath;
    std::map<std::string, std::string> tokens;
    std::string userAgent;
    size_t connectTimeout;
    size_t executeTimeout;
    std::string range;
    bool HttpGet(CURL * curl, const char* url, const char* language, const std::string& input);
  public:
    HttpClient();
    virtual ~HttpClient();
    bool HttpGet(const char* url, Json::Value& json, const char* language);
    bool HttpGet(const char* url, std::stringstream& header, std::stringstream& body, const char* language);
    bool HttpGet(const char* url, std::stringstream& header, std::stringstream& body, const char* language, const std::string& input);
    bool HttpGet(const char* url, curl_callback headerFunc = NULL, curl_callback dataFunc = NULL, void* userdata = NULL);
    bool HttpPost(const std::string& url, const Json::Value& input, Json::Value& output, const std::string& language);
    void SetSecurityToken(const char* securityToken);
    void SetAccessToken(const char* accessToken);
    void SetDomain(const char* domain);
    const char* SecurityToken();
    const char* Domain();

    static char * EncodeStr(const char* str);
    static void FreeEncodedStr(char * str);
    void SetConnectTimeout(size_t seconds);
    void SetExecTimeout(size_t seconds);
    void SetRange(const char* range);

    void SetClientCert(const char* certPath, const char* keyPath)
    {
      this->certPath = certPath;
      this->keyPath = keyPath;
    }

    void SetCACert(const char* caPath)
    {
      this->caPath = caPath;
    }

    void SetUserAgent(const char* userAgent)
    {
      this->userAgent = userAgent;
    }

  private:
    static size_t WriteToString(void* ptr, size_t size, size_t nmemb, void* str);
    void SetToken(const char* name, const char* token);
    const char* GetToken(const char* name);

#if !(defined(_WIN32) || defined(_WIN64))
    static curl_socket_t CurlOpenSocketCallback(void * clientp, curlsocktype purpose, struct curl_sockaddr * addr);
#endif
  };
}
