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

#include "BlobApi.h"
#include "commands/MvCommand.h"


namespace bdblob
{
  extern BlobApi * g_api;


  MvCommand::MvCommand(std::string prefix)
    : Command("mv", std::move(prefix))
  {
    this->parser.Register<bool>("force", false, 'f', "overwrite existing file", false, false);
    this->parser.Register<std::string>("source", true, 0, "source path", true, std::string());
    this->parser.Register<std::string>("destination", true, 0, "destination path", true, std::string());
  }


  int MvCommand::Execute()
  {
    assert(g_api);

    bool force = this->parser["force"].AsBoolean();
    std::string source = this->parser["source"].AsString();
    std::string target = this->parser["destination"].AsString();

    // TODO: support mv wildcard filenames
    if (!g_api->Move(std::move(source), std::move(target), force))
    {
      fprintf(stderr, "%s\n", BlobErrorToString(g_api->Error()));
      return static_cast<int>(g_api->Error());
    }

    return 0;
  }
}