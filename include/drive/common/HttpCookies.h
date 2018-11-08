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
#include <map>
#include <vector>
#include <drive/common/Lock.h>

namespace bdfs
{
  class HttpCookies
  {
    class Cookie
    {
    private:
      std::string name;
      std::string value;
      std::string domain;
      std::string path;
      time_t expires;
      uint32_t maxAge;
      bool httpOnly;
      bool secure;

    public:
      Cookie();
      Cookie(const char * name, const char * value, const char * domain, const char * path, time_t expires, uint32_t maxAge, bool httpOnly, bool secure);

      std::string & Name() { return name; }
      void Name(std::string & value) { name = value; }
      std::string & Value() { return value; }
      void Value(std::string & value) { this->value = value; }
      std::string & Domain() { return domain; }
      void Domain(std::string & value) { domain = value; }
      std::string & Path() { return path; }
      void Path(std::string & value) { path = value; }
      time_t Expires() { return expires; }
      void Expires(time_t value) { expires = value; }
      uint32_t MaxAge() { return maxAge; }
      void MaxAge(uint32_t value) { maxAge = value; }
      bool HttpOnly() { return httpOnly; }
      void HttpOnly(bool value) { httpOnly = value; }
      bool Secure() { return secure; }
      void Secure(bool value) { secure = value; }
    };

    typedef std::vector<Cookie*>::iterator Iterator;

  private:
    SharedMutex mutex;
    std::vector<Cookie*> cookies;
    Iterator __Find(Iterator from, const char * name, const char * domain, const char * path);
    void __Delete(const char * name, const char * domain, const char * path);

  public:
    Iterator Begin() { return cookies.begin(); }
    Iterator End() { return cookies.end(); }

    void Set(const char * rawValue, std::string & actualDomain, std::string & actualPath);
    void Set(const char * name, const char * value, const char * domain, const char * path, time_t expires, uint32_t maxAge, bool secure, bool httpOnly);
    void Delete(const char * name, const char * domain, const char * path);

    Iterator FindFirst(const char * name, const char * domain, const char * path);
    Iterator FindNext(Iterator from, const char * name, const char * domain, const char * path);

    std::string Format(const char * domain, const char * path);
  };
}
