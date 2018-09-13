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

#include <time.h>
#include "BlobConfig.h"
#include "BlobApi.h"
#include "commands/ListCommand.h"

namespace bdblob
{
  extern BlobApi * g_api;


  ListCommand::ListCommand(std::string prefix)
    : Command("ls", std::move(prefix))
  {
    this->parser.Register<bool>("list", false, 'l', "show the result in detail list", false, false);
    this->parser.Register<bool>("all", false, 'a', "show hidden files", false, false);
    this->parser.Register<std::string>("path", false, 0, "path", true, "/");
  }


  int ListCommand::Execute()
  {
    assert(g_api);

    bool showList = this->parser["list"].AsBoolean();
    bool showAll = this->parser["all"].AsBoolean();
    std::string path = this->parser["path"].AsString();

    auto parts = BlobApi::SplitPath(path);

    std::string target;
    bool wildcard = false;
    if (parts.size() > 0 && parts[parts.size() - 1].find_first_of("*?") != std::string::npos)
    {
      target = BlobApi::JoinPaths(parts, 0, parts.size() - 1);
      wildcard = true;
    }
    else
    {
      target = path;
    }

    bool needNewLine = false;
    auto result = g_api->List(target);
    if (g_api->Error() != BlobError::SUCCESS)
    {
      fprintf(stderr, "%s\n", BlobErrorToString(g_api->Error()));
      return static_cast<int>(g_api->Error());
    }

    for (const auto & pair : result)
    {
      if (pair.first[0] == '.' && !showAll)
      {
        continue;
      }

      if (wildcard && !BlobApi::PatternMatch(parts[parts.size() - 1], pair.first))
      {
        continue;
      }

      if (!showList)
      {
        printf("%s\t", pair.first.c_str());
        needNewLine = true;
      }
      else
      {
        printf(pair.second.type == Folder::EntryType::FOLDER ? "D " : "F ");

        printf("%20llu ", (long long unsigned)pair.second.size);

        time_t modifytime = pair.second.modifyTime;
        struct tm ts;
        localtime_r(&modifytime, &ts);
        printf("%d-%02d-%02d %02d:%02d:%02d ", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);

        printf("%s\n", pair.first.c_str());
      }
    }

    if (needNewLine)
    {
      printf("\n");
    }

    return 0;
  }
}