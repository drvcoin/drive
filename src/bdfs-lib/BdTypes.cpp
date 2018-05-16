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

#include <assert.h>
#include "BdTypes.h"

namespace bdfs
{
  std::map<std::string, BdTypes::NewFunc> * BdTypes::registrations = nullptr;

  
  int BdTypes::Register(const char * type, BdTypes::NewFunc callback)
  {
    assert(type && callback);

    if (!registrations)
    {
      registrations = new std::map<std::string, BdTypes::NewFunc>();
    }

    (*registrations)[type] = callback;

    return static_cast<int>(registrations->size());
  }


  std::shared_ptr<BdObject> BdTypes::Create(const char * base, const char * name, const char * path, const char * type)
  {
    if (!base || !name || !path || !path || !registrations)
    {
      return nullptr;
    }

    auto itr = registrations->find(type);
    if (itr != registrations->end())
    {
      return itr->second(base, name, path, type);
    }

    return std::shared_ptr<BdObject>(new BdObject(base, name, path, type));
  }
}
