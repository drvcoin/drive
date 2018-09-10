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

#include <stdio.h>
#include <string>
#include <vector>
#include "BlobConfig.h"
#include "FileBlobProvider.h"
#include "BlobApi.h"
#include "commands/GetCommand.h"
#include "commands/ListCommand.h"
#include "commands/MkdirCommand.h"
#include "commands/PutCommand.h"
#include "commands/RmCommand.h"

namespace bdblob
{
  BlobApi * g_api = nullptr;
}


static void PrintUsage(const char * prefix)
{
  printf("Usage: %s <action>\n", prefix);
  printf("\n");
  printf("Actions:\n");
  printf("  get\t\t\tDownload file.\n");
  printf("  ls\t\t\tList files or directories.\n");
  printf("  mkdir\t\t\tCreate directory.\n");
  printf("  rm\t\t\tDelete files or directories.\n");
  printf("  put\t\t\tUpload file.\n");
}


int main(int argc, const char ** argv)
{
  if (argc < 2)
  {
    PrintUsage(argv[0]);
    return -1;
  }

  bdblob::BlobConfig::Initialize();

  auto provider = std::unique_ptr<bdblob::BlobProvider>(new bdblob::FileBlobProvider("storage"));
  bdblob::g_api = new bdblob::BlobApi();
  bdblob::g_api->Initialize(std::move(provider), bdblob::BlobConfig::RootId());

  std::vector<std::unique_ptr<bdblob::Command>> commands;
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::GetCommand(argv[0])));
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::ListCommand(argv[0])));
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::MkdirCommand(argv[0])));
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::PutCommand(argv[0])));
  commands.emplace_back(std::unique_ptr<bdblob::Command>(new bdblob::RmCommand(argv[0])));

  for (const auto & command : commands)
  {
    if (command->Name() == argv[1])
    {
      return command->Execute(argc - 1, &argv[1]);
    }
  }

  PrintUsage(argv[0]);
  return -1;  
}