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

#include <memory>
#include "BlobConfig.h"
#include "BlobApi.h"
#include "commands/CatCommand.h"

namespace bdblob
{
  extern BlobApi * g_api;


  CatCommand::CatCommand(std::string prefix)
    : Command("cat", std::move(prefix))
  {
    this->parser.Register<std::string>("path", true, 0, "path", true, std::string());
  }


  int CatCommand::Execute(int argc, const char ** argv)
  {
    assert(g_api);

    if (!this->parser.Parse(argc, argv))
    {
      fprintf(stderr, "Invalid argument.\n");
      this->PrintUsage();
      return -1;
    }

    std::string path = this->parser.GetArgument("path")->AsString();

    uint64_t offset = 0;
    uint64_t size = BlobConfig::BlockSize();
    std::unique_ptr<char[]> buffer(new char[size + 1]);
    while (g_api->Get(path, offset, buffer.get(), &size) && size > 0)
    {
      buffer.get()[size] = '\0';
      printf("%s", buffer.get());
      offset += size;
      size = BlobConfig::BlockSize();
    }

    if (g_api->Error() != BlobError::SUCCESS)
    {
      fprintf(stderr, "%s\n", BlobErrorToString(g_api->Error()));
      return static_cast<int>(g_api->Error());
    }

    return 0;
  }
}