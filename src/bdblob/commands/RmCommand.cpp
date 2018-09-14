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
#include "commands/RmCommand.h"

namespace bdblob
{
  extern BlobApi * g_api;


  RmCommand::RmCommand(std::string prefix)
    : Command("rm", std::move(prefix))
  {
    this->parser.Register<bool>("recursive", false, 'r', "delete files or directories", false, false);
    this->parser.Register<bool>("force", false, 'f', "delete even if directory is not empty", false, false);
    this->parser.Register<std::string>("target", true, 0, "target", true, "");
  }


  int RmCommand::Execute(int argc, const char ** argv)
  {
    assert(g_api);

    if (!this->parser.Parse(argc, argv))
    {
      fprintf(stderr, "Invalid arguments.\n");
      this->PrintUsage();
      return -1;
    }

    bool recursive = this->parser.GetArgument("recursive")->AsBoolean();
    bool force = this->parser.GetArgument("force")->AsBoolean();
    std::string target = this->parser.GetArgument("target")->AsString();

    auto parts = BlobApi::SplitPath(target);
    auto parent = g_api->GetParentFolder(parts);
    if (!parent)
    {
      return 0;
    }

    std::string parentPath;
    if (parts.size() > 0)
    {
      parentPath = BlobApi::JoinPaths(parts, 0, parts.size() - 1);
    }

    if (parentPath == "/")
    {
      parentPath = std::string();
    }

    auto matched = parent->Find(BlobApi::BaseName(target));
    for (const auto & name : matched)
    {
      std::string path = parentPath + "/" + name;
      int rtn = this->Delete(path, recursive, force);
      if (rtn != 0)
      {
        return rtn;
      }
    }

    return 0;
  }


  int RmCommand::Delete(const std::string & path, bool recursive, bool force)
  {
    auto info = g_api->Info(path);
    if (info->type == Folder::EntryType::FOLDER && !recursive)
    {
      fprintf(stderr, "%s\n", BlobErrorToString(BlobError::REJECT_DELETE_FOLDER));
      return static_cast<int>(BlobError::REJECT_DELETE_FOLDER);
    }

    if (!g_api->Delete(path))
    {
      if (g_api->Error() != BlobError::FOLDER_NOT_EMPTY || !force)
      {
        fprintf(stderr, "%s\n", BlobErrorToString(g_api->Error()));
        return static_cast<int>(g_api->Error());
      }

      auto children = g_api->List(path);
      for (const auto & child : children)
      {
        if (child.first == "." || child.first == "..")
        {
          continue;
        }

        std::string childPath = path + "/" + child.first;
        int rtn = this->Delete(childPath, recursive, force);
        if (rtn != 0)
        {
          return rtn;
        }
      }

      if (!g_api->Delete(path))
      {
        fprintf(stderr, "%s\n", BlobErrorToString(g_api->Error()));
        return static_cast<int>(g_api->Error());
      }
    }

    return 0;
  }
}