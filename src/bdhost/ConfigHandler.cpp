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

#include <sstream>
#include "Options.h"
#include "Util.h"
#include "HttpHandlerRegister.h"
#include "ConfigHandler.h"

namespace bdhost
{
  static const char PATH[] = "/api/host/Config";

  REGISTER_HTTP_HANDLER(Config, PATH, new ConfigHandler());

  void ConfigHandler::ProcessRequest(bdhttp::HttpContext & context)
  {
    std::string path = context.path();
    if (path.size() <= sizeof(PATH) || path[sizeof(PATH) - 1] != '/')
    {
      if (path.size() == sizeof(PATH) && path == std::string(PATH) + '/')
      {
        context.writeResponse("{\"Name\":\"Config\", \"Path\":\"host://Config\", \"Type\":\"Config\"}");
      }
      else
      {
        context.setResponseCode(404);
        context.writeError("Failed", "Object not found", bdhttp::ErrorCode::OBJECT_NOT_FOUND);
      }

      return;
    }

    auto pos = path.find('/', sizeof(PATH));
    if (pos != std::string::npos)
    {
      context.setResponseCode(404);
      context.writeError("Failed", "Object not found", bdhttp::ErrorCode::OBJECT_NOT_FOUND);
      return;
    }

    auto action = path.substr(sizeof(PATH));

    if (action == "GetStatistics")
    {
      this->OnGetStatistics(context);
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Action not supported", bdhttp::ErrorCode::NOT_SUPPORTED);
    }
  }


  void ConfigHandler::OnGetStatistics(bdhttp::HttpContext & context)
  {
    auto availableSize = Options::size - GetReservedSpace();

    std::stringstream ss;
    ss << "{\"AvailableSize\":" << availableSize << ",\"TotalSize\":" << Options::size << "}";

    context.writeResponse(ss.str());
  }
}