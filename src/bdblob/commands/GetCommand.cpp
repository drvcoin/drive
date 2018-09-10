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

#include <sys/stat.h>
#include "BlobApi.h"
#include "commands/GetCommand.h"


namespace bdblob
{
  extern BlobApi * g_api;


  GetCommand::GetCommand(std::string prefix)
    : Command("get", std::move(prefix))
  {
    this->parser.Register("force", false, "f", "overwrite existing local files", false, false);
    this->parser.Register("source", Argument::Type::STRING, true, "", "remote source", true);
    this->parser.Register("target", false, "", "local target", ".", true);
  }


  int GetCommand::Execute(int argc, const char ** argv)
  {
    assert(g_api);

    if (!this->parser.Parse(argc, argv))
    {
      fprintf(stderr, "Invalid arguments.\n");
      this->PrintUsage();
      return -1;
    }

    bool force = this->parser.GetValue<bool>("force");
    std::string source = this->parser.GetValue<std::string>("source");
    std::string target = this->parser.GetValue<std::string>("target");

    // TODO: support downloading folders and wildcard filenames
    auto info = g_api->Info(source);
    if (!info)
    {
      fprintf(stderr, "%s\n", BlobErrorToString(g_api->Error()));
      return static_cast<int>(g_api->Error());
    }

    if (info->type == Folder::EntryType::FOLDER)
    {
      fprintf(stderr, "%s\n", BlobErrorToString(BlobError::NOT_SUPPORTED));
      return static_cast<int>(BlobError::NOT_SUPPORTED);
    }

    if (target.empty())
    {
      target = ".";
    }

    struct stat props;
    if (stat(target.c_str(), &props) == 0 && S_ISDIR(props.st_mode))
    {
      target += (target[target.size() - 1] == '/' ? "" : "/") + BlobApi::BaseName(source);
    }

    if (!g_api->Get(source, target, force))
    {
      fprintf(stderr, "%s\n", BlobErrorToString(g_api->Error()));
      return static_cast<int>(g_api->Error());
    }

    return 0;
  }
}