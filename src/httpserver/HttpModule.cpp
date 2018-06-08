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

#include "HttpModule.h"
#include "HttpHandler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <time.h>
#include <stdarg.h>

#include <map>
#include <string>

namespace bdhttp
{
  std::map<std::string,HttpModule::HandlerEntry>* HttpModule::handlers = NULL;

  void HttpModule::RegisterHandler(std::string path, std::string verb, HttpHandler* handler)
  {
    HandlerEntry entry;
    entry.verb = verb;
    entry.handler = handler;

    if (handlers == NULL)
    {
      handlers = new std::map<std::string,HttpModule::HandlerEntry>();
    }

    handlers->insert(std::pair<std::string, HandlerEntry>(path, entry));
  }

  void HttpModule::Initialize(void)
  {
    for(std::map<std::string, HandlerEntry>::const_iterator it =
          handlers->begin();
        it != handlers->end();
        it++)
    {
      HandlerEntry entry = it->second;

      entry.handler->Initialize();
    }
  }

  void HttpModule::Stop(void)
  {
    for(std::map<std::string, HandlerEntry>::const_iterator it =
          handlers->begin();
        it != handlers->end();
        it++)
    {
      HandlerEntry entry = it->second;

      entry.handler->Stop();
    }
  }

  bool HttpModule::Dispatch(HttpContext & context)
  {
    std::string path = context.path();
    size_t pos = std::string::npos;

    do
    {
      for(std::map<std::string, HandlerEntry>::const_iterator it =
          handlers->find(path.substr(0, pos));
          it != handlers->end();
          it++)
      {
        HandlerEntry entry = it->second;

        if (entry.verb == "*" || context.verb() == entry.verb)
        {
          entry.handler->ProcessRequest(context);
          return true;
        }
      }

      pos = path.find_last_of('/', pos - 1);
    } while(pos != 0 && pos != std::string::npos);

    return false;
  }
}
