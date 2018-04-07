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

#include "Action.h"

#include <string.h>

namespace dfs
{
  namespace Action
  {
    const char * ToString(T value)
    {
      switch (value)
      {
        default:
        case Unknown: return "Unknown";
        case Create:  return "Create";
        case Delete:  return "Delete";
        case Verify:  return "Verify";
        case Resize:  return "Resize";
        case Mount:   return "Mount";
        case Unmount: return "Unmount";
        case Daemon:  return "Daemon";
      }
    }

    T FromString(const char * value)
    {
      if (value != NULL)
      {
             if (strcasecmp("Create", value) == 0)  { return Create; }
        else if (strcasecmp("Delete", value) == 0)  { return Delete; }
        else if (strcasecmp("Verify", value) == 0)  { return Verify; }
        else if (strcasecmp("Resize", value) == 0)  { return Resize; }
        else if (strcasecmp("Mount", value) == 0)   { return Mount; }
        else if (strcasecmp("Unmount", value) == 0) { return Unmount; }
        else if (strcasecmp("Daemon", value) == 0)  { return Daemon; }
      }
      return Unknown;
    }
  }
}
