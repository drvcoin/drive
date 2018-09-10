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
#include "BlobConfig.h"

namespace bdblob
{
  uint64_t BlobConfig::blockSize = 64 * 1024;

  std::string * BlobConfig::rootId = nullptr;


  void BlobConfig::SetRootId(std::string val)
  {
    if (*rootId != val)
    {
      // TODO: move this into /etc/...
      FILE * file = fopen("blob_root", "w");
      if (file)
      {
        fwrite(val.c_str(), 1, val.size(), file);
        fclose(file);
      }

      *rootId = std::move(val);
    }
  }


  void BlobConfig::Initialize()
  {
    rootId = new std::string();

    FILE * file = fopen("blob_root", "r");
    if (file)
    {
      fseek(file, 0, SEEK_END);
      long size = ftell(file);
      rewind(file);

      char * buffer = new char[size + 1];
      fread(buffer, 1, size, file);
      fclose(file);

      buffer[size] = '\0';

      *rootId = buffer;
      delete[] buffer;
    }
  }
}