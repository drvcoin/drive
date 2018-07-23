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

#include "HttpCookies.h"
#include "RelayInfo.h"
#include <string>
#include <vector>

namespace bdfs
{
  class HttpConfig
  {
  private:
    HttpCookies cookies;
    uint32_t connectTimeout = 5;
    uint32_t requestTimeout = 5;
    std::string userAgent;
    std::string certPath;
    std::string keyPath;
    std::string caPath;

    std::vector<RelayInfo> relays;
    int activeRelay = -1;
    int activeRelayEndpoint = -1;

  public:
    HttpCookies & Cookies() { return cookies; }
    uint32_t ConnectTimeout() { return connectTimeout; }
    void ConnectTimeout(uint32_t value) { connectTimeout = value; }
    uint32_t RequestTimeout() { return requestTimeout; }
    void RequestTimeout(uint32_t value) { requestTimeout = value; }
    std::string & UserAgent() { return userAgent; }
    void UserAgent(std::string & value) { userAgent = value; }
    void UserAgent(const char * value) { userAgent = value; }
    std::string & CertPath() { return certPath; }
    void CertPath(std::string & value) { certPath = value; }
    void CertPath(const char * value) { certPath = value; }
    std::string & KeyPath() { return keyPath; }
    void KeyPath(std::string & value) { keyPath = value; }
    void KeyPath(const char * value) { keyPath = value; }
    std::string & CaPath() { return caPath; }
    void CaPath(std::string & value) { caPath = value; }
    void CaPath(const char * value) { caPath = value; }

    std::vector<RelayInfo> & Relays()         { return relays; }
    int ActiveRelay()                         { return activeRelay; }
    int ActiveRelayEndpoint()                 { return activeRelayEndpoint; }
    void Relays(std::vector<RelayInfo> val)   { relays = std::move(val); }
    void ActiveRelay(int val)                 { activeRelay = val; }
    void ActiveRelayEndpoint(int val)         { activeRelayEndpoint = val; }
  };
}
