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

#include <vector>
#include "HttpModule.h"
#include "MGHttpContext.h"

#include "HttpServer.h"

#define CALLBACK_RESULT_NOT_HANDLED 0
#define CALLBACK_RESULT_HANDLED 1


namespace bdhost {

  HttpServer::HttpServer()
    : context(nullptr) {
  }


  HttpServer::~HttpServer() {
    this->Stop();
  }


  bool HttpServer::Start(uint16_t port, const char * certificate) {
    if (this->IsRunning()) {
      return false;
    }

    char portStr[10];
    if (certificate)
    {
      sprintf(portStr, "%us", port);
    }
    else
    {
      sprintf(portStr, "%u", port);
    }

    std::vector<const char *> options;

    options.push_back("listening_ports");
    options.push_back(portStr);

    options.push_back("num_threads");
    options.push_back("10");

    options.push_back("enable_directory_listing");
    options.push_back("no");

    options.push_back("enable_keep_alive");
    options.push_back("yes");

    if (certificate)
    {
      options.push_back("ssl_ceritificate");
      options.push_back(certificate);
    }

    options.push_back(nullptr);

    mg_callbacks callbacks = {};
    callbacks.begin_request = & HttpServer::OnRequest;

    this->context = mg_start(& callbacks, this, &options[0]);

    return this->IsRunning();
  }


  void HttpServer::Stop() {
    if (this->IsRunning()) {
      mg_stop(this->context);
      this->context = nullptr;
    }
  }


  bool HttpServer::IsRunning() {
    return this->context != nullptr;
  }


  int HttpServer::OnRequest(mg_connection * connection) {
    mg_request_info * ri = mg_get_request_info(connection);
    if (!ri) {
      return CALLBACK_RESULT_NOT_HANDLED;
    }

    // HttpServer * _this = static_cast<HttpServer *>(ri->user_data);

    MGHttpContext context(connection, ri, false);
    if (!HttpModule::Dispatch(context)) {
      context.setResponseCode(404);
      context.writeResponse(nullptr, 0);
    }

    return CALLBACK_RESULT_HANDLED;
  }
}
