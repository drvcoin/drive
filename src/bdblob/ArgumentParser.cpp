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
#include <stdlib.h>
#include <string.h>
#include "ArgumentParser.h"

namespace bdblob
{
  void ArgumentParser::PrintUsage(std::string prefix) const
  {
    printf("Usage: %s [options]", prefix.c_str());
    for (const auto & name : this->anonymouse)
    {
      auto itr = this->args.find(name);
      if (itr != this->args.end())
      {
        printf(" ");
        printf(itr->second->IsRequired() ? "<" : "[");
        printf("%s", itr->second->Description().c_str());
        printf(itr->second->IsRequired() ? ">" : "]");
      }
    }

    printf("\n");

    bool firstOption = true;
    for (const auto & pair : this->args)
    {
      if (pair.second->IsAnonymouse())
      {
        continue;
      }

      if (firstOption)
      {
        firstOption = false;

        printf("\n");
        printf("Options:\n");
      }

      printf("  ");
      if (pair.second->Abbrev() != 0)
      {
        printf("-%c, ", pair.second->Abbrev());
      }
      else
      {
        printf("    ");
      }

      printf("--%-20s", pair.first.c_str());

      if (pair.second->IsRequired())
      {
        printf("%s%s\n", "(Required)", pair.second->Description().c_str());
      }
      else
      {
        printf("%s\n", pair.second->Description().c_str());
      }
    }
  }


  static void parseParam(std::string input, std::string & key, std::string & value)
  {
    if (input.empty())
    {
      return;
    }

    auto pos = input.find_first_of('=');
    if (pos == 0 || pos == input.size() - 1)
    {
      return;
    }
    else if (pos != std::string::npos)
    {
      key = input.substr(0, pos);
      value = input.substr(pos + 1, input.size() - pos - 1);
    }
    else
    {
      key = std::move(input);
    }
  }


  bool ArgumentParser::Parse(int argc, const char ** argv, std::map<std::string, std::string> & params) const
  {
    size_t anonymouseIdx = 0;
    for (int idx = 1; idx < argc; ++idx)
    {
      std::string key;
      std::string value;
      const char * cur = argv[idx];

      if (cur[0] != '-')
      {
        if (anonymouseIdx >= this->anonymouse.size())
        {
          return false;
        }

        params[this->anonymouse[anonymouseIdx++]] = cur;
      }
      else
      {
        bool isAbbrev = cur[1] != '-';
        std::string key, value;
        parseParam(isAbbrev ? cur + 1 : cur + 2, key, value);

        if (key.empty() || (isAbbrev && key.size() > 1 && !value.empty()))
        {
          return false;
        }

        std::vector<std::string> names;
        if (!isAbbrev)
        {
          names.emplace_back(std::move(key));
        }
        else
        {
          for (size_t i = 0; i < key.size(); ++i)
          {
            auto itr = this->abbrevs.find(key[i]);
            if (itr == this->abbrevs.end())
            {
              return false;
            }

            names.emplace_back(itr->second);
          }
        }

        for (const auto & name : names)
        {
          auto itr = this->args.find(name);
          if (itr == this->args.end())
          {
            return false;
          }

          if (names.size() > 1 && !itr->second->IsBoolean())
          {
            return false;
          }

          if (itr->second->IsBoolean())
          {
            params[name] = value;
          }
          else
          {
            if (!value.empty())
            {
              params[name] = value;
            }
            else if (idx < argc - 1)
            {
              params[name] = argv[++idx];
            }
            else
            {
              return false;
            }
          }
        }
      }
    }

    return true;
  }


  bool ArgumentParser::Parse(int argc, const char ** argv)
  {
    std::map<std::string, std::string> params;
    if (!this->Parse(argc, argv, params))
    {
      return false;
    }

    for (const auto & param : params)
    {
      auto itr = this->args.find(param.first);
      if (itr == this->args.end())
      {
        return false;
      }

      if (itr->second->IsString())
      {
        auto ptr = static_cast<StringArgument *>(itr->second.get());
        ptr->SetValue(param.second);
      }
      else if (itr->second->IsIntegral())
      {
        auto ptr = static_cast<IntArgument *>(itr->second.get());
        ptr->SetValue((int64_t)atoll(param.second.c_str()));
      }
      else // Boolean
      {
        auto ptr = static_cast<BoolArgument *>(itr->second.get());
        ptr->SetValue(param.second.empty() || param.second == "true" || param.second == "1");
      }
    }

    return this->ValidateRequired();
  }


  bool ArgumentParser::IsSet(std::string name) const
  {
    auto itr = this->args.find(std::move(name));
    if (itr == this->args.end())
    {
      return false;
    }

    return itr->second->IsSet();
  }


  bool ArgumentParser::ValidateRequired() const
  {
    for (const auto & pair : this->args)
    {
      if (pair.second->IsRequired() && !pair.second->IsSet())
      {
        return false;
      }
    }

    return true;
  }


  ArgumentBase * ArgumentParser::GetArgument(std::string name) const
  {
    auto itr = this->args.find(std::move(name));
    return itr != this->args.end() ? itr->second.get() : nullptr;
  }


  ArgumentBase & ArgumentParser::operator[](std::string name) const
  {
    return *(this->GetArgument(std::move(name)));
  }
}