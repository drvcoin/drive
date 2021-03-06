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

#include "Command.h"

namespace bdblob
{
  Command::Command(std::string name, std::string prefix)
    : name(std::move(name))
    , prefix(std::move(prefix))
  {
  }


  int Command::Execute(int argc, const char ** argv)
  {
    if (!this->parser.Parse(argc, argv))
    {
      fprintf(stderr, "Invalid arguments.\n\n");
      this->PrintUsage();
      return -1;
    }

    return this->Execute();
  }


  void Command::PrintUsage() const
  {
    std::string command;
    if (!this->prefix.empty())
    {
      command = this->prefix;
    }

    if (!this->name.empty())
    {
      command += command.empty() ? this->name : " " + this->name;
    }

    this->parser.PrintUsage(std::move(command));
  }
}