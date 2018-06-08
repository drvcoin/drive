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

#include "HttpModule.h"
#include "HttpHandler.h"
#include <stdio.h>

namespace bdhttp
{
  // TODO: convert to template to ensure type safe
  class HttpHandlerRegister
  {
  public:
    HttpHandlerRegister(const char* path, const char* verb, HttpHandler* handler)
    {
      HttpModule::RegisterHandler(path, verb, handler);
    };
  };

#define REGISTER_HTTP_HANDLER_VERB(name, path, verb, handler) bdhttp::HttpHandlerRegister __HanderREG__##name(path, verb, handler)
#define REGISTER_HTTP_HANDLER(name, path, handler) bdhttp::HttpHandlerRegister __HanderREG__##name(path, "*", handler)
}
