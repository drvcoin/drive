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

#pragma once
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>
#pragma pack(push,1)
namespace bdfs
{
  // Block Drive Client Protocol
  namespace bdcp
  {
    using namespace std;

    enum T : uint8_t
    {
      BIND = 0,
      UNBIND,
      RESPONSE,
      QUERY_VOLUMEINFO
    };

    // paramCount = null terminated params after struct
    typedef struct
    {
      uint32_t length;
      T type;
      uint8_t paramCount;
    }BdHdr;

    typedef struct
    {
      BdHdr hdr;
    }BdRequest;

    // BdResponse for BIND
    // status = 0: Fail, 1: Success, 2: Already Binded
    // data = nbdpath or error message
    typedef struct
    {
      BdHdr hdr;
      uint8_t status;
    }BdResponse;

    unique_ptr<char[]> Create(T type, const vector<string> &args, uint8_t status = 0)
    {
      const uint32_t structSize = (type == RESPONSE ? sizeof(BdResponse) : sizeof(BdRequest));
      uint32_t size = structSize;
      for(auto &arg : args)
      {
        size += arg.length() + 1;
      }
      
      unique_ptr<char[]> ptr = make_unique<char[]>(size);
      BdHdr *pHdr = (BdHdr *)ptr.get();
      pHdr->type = type;
      pHdr->length = size;
      pHdr->paramCount = args.size();

      if(type == RESPONSE)
      {
       ((BdResponse *)ptr.get())->status = status;
      }
      
      uint32_t pos = structSize;
      char *p = ptr.get();
      for(auto &arg : args)
      {
        strncpy(p+pos,arg.c_str(),arg.length()+1);
        pos += arg.length()+1;
      }
      
      return ptr;
    }

    vector<string> Parse(unique_ptr<char []> &ptr)
    {
      vector<string> args;
      BdHdr *pHdr = (BdHdr *)ptr.get();

      const char *p = ptr.get();
      uint32_t pos = (((BdHdr*)p)->type == RESPONSE ? sizeof(BdResponse) : sizeof(BdRequest));
      for(int i = 0; i < pHdr->paramCount; i++)
      {
        string str = string(p+pos);
        pos += str.length() + 1;
        args.emplace_back(str);
      }
      return args;
    }
  }
}
#pragma pack(pop)
