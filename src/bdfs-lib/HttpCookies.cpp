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

#include "HttpCookies.h"

#include <algorithm>
#include <sstream>
#include <string.h>

namespace bdfs
{

  HttpCookies::Cookie::Cookie() :
    expires(0),
    maxAge(0),
    httpOnly(false),
    secure(false)
  {
  }

  HttpCookies::Cookie::Cookie(const char * name, const char * value, const char * domain, const char * path, time_t expires, uint32_t maxAge, bool httpOnly, bool secure) :
    name(name?name:""),
    value(value?value:""),
    domain(domain?domain:""),
    path(path?path:""),
    expires(expires),
    maxAge(maxAge),
    httpOnly(httpOnly),
    secure(secure)
  {
  }

  void HttpCookies::Set(const char * str, std::string & actualDomain, std::string & actualPath)
  {
    const char * equals = strchr(str, '=');

    if (equals != NULL)
    {
      Cookie * cookie = new Cookie();
      std::string name(str, equals-str);
      str = equals;
      while (*(++str)==' ');
      const char * semi = strchrnul(str, ';');
      std::string value(str, semi-str);
      str = semi;
      cookie->Name(name);
      cookie->Value(value);
      cookie->Domain(actualDomain);
      cookie->Path(actualPath);
      while (*str != '\0')
      {
        while (*(++str)==' ');
        const char * equals = strchrnul(str, '=');
        const char * semi = strchrnul(str, ';');
        if (equals < semi) // value option
        {
          std::string optionName(str, equals-str);
          str = equals;
          while (*(++str)==' ');
          std::string optionValue(str, semi-str);
          std::transform(optionName.begin(), optionName.end(), optionName.begin(), ::tolower);
          if (optionName == "domain")
          {
            if (std::equal(optionValue.begin(), optionValue.end(), cookie->Domain().end() - optionValue.size()))
            {
              cookie->Domain(optionValue);
            }
          }
          else if (optionName == "path")
          {
            if (std::equal(optionValue.begin(), optionValue.end(), cookie->Path().begin()))
            {
              cookie->Path(optionValue);
            }
          }
          else if (optionName == "max-age")
          {
            // TODO: parse this
            cookie->MaxAge(0);
          }
          else if (optionName == "expires")
          {
            // TODO: parse this
            cookie->Expires(0);
          }
        }
        else // option
        {
          std::string option(str, semi-str);
          std::transform(option.begin(), option.end(), option.begin(), ::tolower);
          if (option == "http-only")
          {
            cookie->HttpOnly(true);
          }
          else if (option == "secure")
          {
            cookie->Secure(true);
          }
        }
        str = semi;
      }
      WriteLock _(mutex);
      __Delete(name.c_str(), cookie->Domain().c_str(), cookie->Path().c_str());
      cookies.push_back(cookie);
    }
  }

  void HttpCookies::Set(const char * name, const char * value, const char * domain, const char * path, time_t expires, uint32_t maxAge, bool secure, bool httpOnly)
  {
    WriteLock _(mutex);
    __Delete(name, domain, path);
    cookies.push_back(new Cookie(name, value, domain, path, expires, maxAge, secure, httpOnly));
  }

  void HttpCookies::Delete(const char * name, const char * domain, const char * path)
  {
    WriteLock _(mutex);
    __Delete(name, domain, path);
  }

  void HttpCookies::__Delete(const char * name, const char * domain, const char * path)
  {
    std::string sname(name?name:"");
    std::string sdomain(domain?domain:"");
    std::string spath(path?path:"");

    for (size_t i = 0; i < cookies.size();)
    {
      Cookie * cookie = cookies[i];
      if (cookie->Name() == sname &&
          cookie->Domain() == sdomain &&
          cookie->Path() == spath)
      {
        cookies.erase(cookies.begin() + i);
      }
      else
      {
        i++;
      }
    }
  }

  HttpCookies::Iterator HttpCookies::FindFirst(const char * name, const char * domain, const char * path)
  {
    ReadLock _(mutex);
    return __Find(cookies.begin(), name, domain, path);
  }

  HttpCookies::Iterator HttpCookies::FindNext(HttpCookies::Iterator from, const char * name, const char * domain, const char * path)
  {
    ReadLock _(mutex);
    return __Find(++from, name, domain, path);
  }

  HttpCookies::Iterator HttpCookies::__Find(HttpCookies::Iterator begin, const char * name, const char * domain, const char * path)
  {
    std::string sname(name?name:"");
    std::string sdomain(domain?domain:"");
    std::string spath(path?path:"");

    return std::find_if(begin, cookies.end(), [&](Cookie * cookie) {
      return
        (sname.size() == 0 || cookie->Name() == sname) &&
        (cookie->Domain().size() == 0 || std::equal(cookie->Domain().begin(), cookie->Domain().end(), sdomain.end() - cookie->Domain().size())) &&
        (cookie->Path().size() == 0 || std::equal(cookie->Path().begin(), cookie->Path().end(), spath.begin()));
    });
  }

  std::string HttpCookies::Format(const char * domain, const char * path)
  {
    ReadLock _(mutex);
    std::string sdomain(domain?domain:"");
    std::string spath(path?path:"");
    std::stringstream res;
    for (Iterator it = cookies.begin(); it != cookies.end(); it++)
    {
      Cookie * cookie = (*it);
      if ((cookie->Domain().size() == 0 || std::equal(cookie->Domain().begin(), cookie->Domain().end(), sdomain.end() - cookie->Domain().size())) &&
          (cookie->Path().size() == 0 || std::equal(cookie->Path().begin(), cookie->Path().end(), spath.begin())))
      {
        res << (*it)->Name() << '=' << (*it)->Value() << ';';
      }
    }
    return res.str();
  }

}
