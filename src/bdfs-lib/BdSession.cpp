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

#include "BdSession.h"
#include "HttpRequest.h"
#include "BdObject.h"
#include "BdTypes.h"
#include "AsyncQueue.h"

#include <sstream>

namespace bdfs
{
  namespace
  {
    void OnDequeue(HttpRequest* & request, void * context)
    {
      printf("Processing: %s\n", request->Url().c_str());
      request->Execute();
    }

    void OnDelete(HttpRequest* & request, void * context)
    {
      delete (request);
    }

    AsyncQueue<HttpRequest*> queue(OnDequeue, OnDelete, NULL);
    static bool queueStarted = false;
  }

  SharedMutex BdSession::mutex;
  HttpConfig BdSession::defaultConfig;
  std::map<std::string, std::shared_ptr<BdSession> > BdSession::sessions;

  std::shared_ptr<BdSession> BdSession::GetSession(const char * base)
  {
    std::string strBase(base);
    return GetSession(strBase);
  }

  std::shared_ptr<BdSession> BdSession::GetSession(std::string & base)
  {
    ReadLock _(mutex);
    return sessions[base];
  }

  std::shared_ptr<BdSession> BdSession::CreateSession(const char * base, HttpConfig * config)
  {
    WriteLock _(mutex);
    if (!queueStarted)
    {
      queue.Start();
      queueStarted = true;
    }
    if (config == NULL)
    {
      config = &defaultConfig;
    }
    std::shared_ptr<BdSession> session(new BdSession(base, config));
    sessions[session->Base()] = session;
    return session;
  }

  BdSession::BdSession(const char * base, HttpConfig * config) :
    config(config),
    base(base)
  {
  }

  std::string BdSession::__EncodeArgs(BdObject::CArgs & args)
  {
    char sep = '?';
    std::stringstream res;
    Json::FastWriter writer; 
    for (BdObject::CArgs::iterator itr = args.begin(); itr != args.end(); itr++)
    {
      res << sep << (*itr).first << '=' << HttpRequest::EncodeStr(writer.write((*itr).second).c_str());
      sep = '&';
    }
    return res.str();
  }

  bool BdSession::Call(std::string & path, const char * method, BdObject::CArgs & args, BdObject::Callback callback)
  {
    std::string pathCopy = path;
    std::stringstream ss;
    ss << base;
    ss << "/api/";
    std::size_t css = path.find("://");
    if (css == std::string::npos) { return false; }
    ss << pathCopy.replace(css, 2, "", 0);
    ss << '/';
    ss << method;
    //ss << this->__EncodeArgs(args);
    std::string url = ss.str();

    Json::Value data;
    for (auto it = args.begin(); it != args.end(); ++it)
    {
      data[it->first] = it->second;
    }

    HttpRequest * req = new HttpRequest(url.c_str(), config);
    if (req != NULL)
    {
      req->Post(data, callback);
      queue.Enqueue(req);
    }
    return true;
  }

  std::shared_ptr<BdObject> BdSession::CreateObject(const char * name, const char * path, const char * type)
  {
    return BdTypes::Create(base.c_str(), name, path, type);
  }
}
