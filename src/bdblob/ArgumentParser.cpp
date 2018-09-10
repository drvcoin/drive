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
  void ArgumentParser::Register(std::string name, Argument::Type type, bool required, std::string abbrev, std::string description, bool isAnonymouse)
  {
    assert(type != Argument::Type::__MAX__);
    assert(!name.empty());
    assert(this->args.find(name) == this->args.end());

    auto ptr = std::make_unique<Argument>(type);

    if (!abbrev.empty())
    {
      assert(this->abbrevs.find(abbrev) == this->abbrevs.end());
      this->abbrevs[abbrev] = name;
      ptr->SetAbbrev(std::move(abbrev));
    }

    ptr->SetDescription(std::move(description));
    ptr->SetRequired(required);
    ptr->SetAnonymouse(isAnonymouse);

    if (isAnonymouse)
    {
      this->anonymouse.emplace_back(name);
    }

    this->args[name] = std::move(ptr);
  }


  void ArgumentParser::Register(std::string name, bool required, std::string abbrev, std::string description, std::string defaultValue, bool isAnonymouse)
  {
    this->Register(name, Argument::Type::STRING, required, std::move(abbrev), std::move(description), isAnonymouse);

    auto itr = this->args.find(std::move(name));
    assert(itr != this->args.end());

    itr->second->SetValue(std::move(defaultValue));
  }

  void ArgumentParser::Register(std::string name, bool required, std::string abbrev, std::string description, const char * defaultValue, bool isAnonymouse)
  {
    this->Register(std::move(name), required, std::move(abbrev), std::move(description), std::string(defaultValue), isAnonymouse);
  }


  void ArgumentParser::Register(std::string name, bool required, std::string abbrev, std::string description, int64_t defaultValue, bool isAnonymouse)
  {
    this->Register(name, Argument::Type::INTEGER, required, std::move(abbrev), std::move(description), isAnonymouse);

    auto itr = this->args.find(std::move(name));
    assert(itr != this->args.end());

    itr->second->SetValue(defaultValue);
  }


  void ArgumentParser::Register(std::string name, bool required, std::string abbrev, std::string description, bool defaultValue, bool isAnonymouse)
  {
    this->Register(name, Argument::Type::BOOLEAN, required, std::move(abbrev), std::move(description), isAnonymouse);

    auto itr = this->args.find(std::move(name));
    assert(itr != this->args.end());

    itr->second->SetValue(defaultValue);
  }


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
      if (!pair.second->Abbrev().empty())
      {
        printf("-%s, ", pair.second->Abbrev().c_str());
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


  bool ArgumentParser::Parse(int argc, const char ** argv)
  {
    size_t anonymouseIdx = 0;
    for (int idx = 1; idx < argc; ++idx)
    {
      const char * param = argv[idx];

      std::string name;

      if (param[0] != '-')
      {
        if (anonymouseIdx >= this->anonymouse.size())
        {
          return false;
        }
        else
        {
          name = this->anonymouse[anonymouseIdx++];
          --idx;
        }
      }
      else
      {
        if (param[1] == '\0')
        {
          return false;
        }

        if (param[1] != '-')
        {
          if (param[2] != '\0')
          {
            return false;
          }

          auto abbrevItr = this->abbrevs.find(&param[1]);
          if (abbrevItr == this->abbrevs.end())
          {
            return false;
          }

          name = abbrevItr->second;
        }
        else
        {
          if (param[2] == '\0')
          {
            return false;
          }

          name = (& param[2]);
        }
      }

      auto itr = this->args.find(name);
      if (itr == this->args.end())
      {
        return false;
      }

      switch (itr->second->GetType())
      {
        case Argument::Type::STRING:
        {
          if (++idx >= argc)
          {
            return false;
          }

          itr->second->SetValue(argv[idx]);
          break;
        }

        case Argument::Type::INTEGER:
        {
          if (++idx >= argc)
          {
            return false;
          }

          itr->second->SetValue((int64_t)atoll(argv[idx]));
          break;
        }

        case Argument::Type::BOOLEAN:
        {
          if (itr->second->IsAnonymouse())
          {
            ++idx;
            itr->second->SetValue(strcmp(param, "true") == 0 || strcmp(param, "1"));
          }
          else
          {
            if (idx == argc - 1 || strcmp(argv[idx + 1], "true") == 0 || strcmp(argv[idx + 1], "1") == 0)
            {
              itr->second->SetValue(true);
              ++idx;
            }
            else if (strcmp(argv[idx + 1], "false") == 0 || strcmp(argv[idx + 1], "0") == 0)
            {
              itr->second->SetValue(false);
              ++idx;
            }
            else
            {
              // This argument acts like a flag
              itr->second->SetValue(true);
            }
          }
          break;
        }

        default:
        {
          return false;
        }
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
}