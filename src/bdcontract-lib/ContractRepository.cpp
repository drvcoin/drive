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

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <direct.h>
#include "dirent.h"

#else
#include <unistd.h>
#include <dirent.h>
#endif

#include "ContractRepository.h"

namespace bdcontract
{
#ifdef _WIN32
  static int mkpath(char* path)
#else
  static int mkpath(char* path, mode_t mode)
#endif
  {
    char * p = path;

    do
    {
      p = strchr(p + 1, '/');
      if (p)
      {
        *p = '\0';
      }

      int rtn = 0;
#ifdef _WIN32
      rtn = _mkdir(path);
#else
      rtn = mkdir(path, mode);
#endif
      if (p)
      {
        *p = '/';
      }

      if (rtn == -1 && errno != EEXIST)
      {
        return -1;
      }
    } while (p);

    return 0;
  }


  ContractRepository::ContractRepository(const char * rootPath)
    : root(rootPath ? rootPath : "")
  {
    if (!root.empty())
    {
      if (root[root.size() - 1] == '/')
      {
        root.resize(root.size() - 1);
      }

#ifdef _WIN32
      mkpath(const_cast<char *>(root.c_str()));
#else
      mkpath(const_cast<char *>(root.c_sssssstr()), 0755);
#endif
    }
    else
    {
      root = ".";
    }
  }


  bool ContractRepository::SaveContract(const Contract & contract) const
  {
    std::string filename = this->root + "/" + contract.Name();

    FILE * file = fopen(filename.c_str(), "w");
    if (!file)
    {
      return false;
    }

    auto str = contract.ToString();
    auto size = fwrite(str.c_str(), 1, str.size(), file);

    fclose(file);

    return size >= str.size();
  }


  std::unique_ptr<Contract> ContractRepository::LoadContract(const char * name) const
  {
    if (!name || name[0] == '\0')
    {
      return nullptr;
    }

    std::string filename = this->root + "/" + name;

    return ContractRepository::Load(filename.c_str());
  }


  std::unique_ptr<Contract> ContractRepository::Load(const char * name)
  {
    if (!name || name[0] == '\0')
    {
      return nullptr;
    }

    std::string filename = name;

    FILE * file = fopen(filename.c_str(), "r");
    if (!file)
    {
      return nullptr;
    }

    size_t size = BUFSIZ;
    char * buffer = static_cast<char *>(malloc(size));
    size_t offset = 0;

    size_t bytes;
    while ((bytes = fread(buffer + offset, 1, size - offset, file)) == size - offset)
    {
      char * buf = static_cast<char *>(realloc(buffer, size + BUFSIZ));
      if (!buf)
      {
        break;
      }

      buffer = buf;
      offset = size;
      size += BUFSIZ;
    }

    fclose(file);

    auto result = Contract::FromString(buffer, offset + bytes);

    free(buffer);

    return std::move(result);
  }


  void ContractRepository::DeleteContract(const char * name) const
  {
    if (!name || name[0] == '\0')
    {
      return;
    }

    std::string filename = this->root + "/" + name;
    _unlink(filename.c_str());
  }


  std::vector<std::string> ContractRepository::GetContractNames() const
  {
    std::vector<std::string> result;

    DIR * dir = opendir(this->root.c_str());
    if (!dir)
    {
      return result;
    }

    struct dirent * ent;
    while ((ent = readdir(dir)) != nullptr)
    {
      if (ent->d_type == DT_REG && ent->d_name[0] != '.')
      {
        result.emplace_back(ent->d_name);
      }
    }

    closedir(dir);

    return std::move(result);
  }
}
