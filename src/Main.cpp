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
#include <sys/stat.h>
#include <cmath>

#include "Options.h"
#include "cm256.h"
#include "gf256.h"
#include "ubd.h"

#include "Volume.h"

using namespace dfs;

static int xmpl_debug = 1;

static size_t xmp_read(void *buf, size_t size, size_t offset, void * context)
{
  return ((Volume*)context)->ReadDecrypt(buf, size, offset) ? 0 : -1;
}

static size_t xmp_write(const void *buf, size_t size, size_t offset, void * context)
{
  return ((Volume*)context)->WriteEncrypt(buf, size, offset) ? 0 : -1;
}

static void xmp_disc(void * context)
{
  (void)(context);
}

static int xmp_flush(void * context)
{
  (void)(context);
  return 0;
}

static int xmp_trim(size_t from, size_t len, void * context)
{
  (void)(context);
  return 0;
}

void ProcessFile(const char * path)
{
  printf("Processing: %s\n", path);

  if (Options::Action == Action::Mount)
  {
    uint64_t blockCount = 256;//*1024*1024ul;
    size_t blockSize = 1*1024*1024;
    std::string volumeId = path;

    Volume volume(volumeId.c_str(), 4, 4, blockCount, blockSize, "HelloWorld");
    volume.SetPartition(0, new Partition("part0", blockCount, blockSize));
    volume.SetPartition(1, new Partition("part1", blockCount, blockSize));
    volume.SetPartition(2, new Partition("part2", blockCount, blockSize));
    volume.SetPartition(3, new Partition("part3", blockCount, blockSize));
    volume.SetPartition(4, new Partition("part4", blockCount, blockSize));
    volume.SetPartition(5, new Partition("part5", blockCount, blockSize));
    volume.SetPartition(6, new Partition("part6", blockCount, blockSize));
    volume.SetPartition(7, new Partition("part7", blockCount, blockSize));
    //volume.SetPartition(8, new Partition("part8", blockCount, blockSize));
    //volume.SetPartition(9, new Partition("part9", blockCount, blockSize));

    static struct ubd_operations ops = {
      .read = xmp_read,
      .write = xmp_write,
      .disc = xmp_disc,
      .flush = xmp_flush,
      .trim = xmp_trim
    };

    ubd_register(path, volume.DataSize(), &ops, (void *)&volume);
  }
}

int main(int argc, char * * argv)
{
  Options::Init(argc, argv);

  if (cm256_init()) {
    exit(1);
  }

  for (std::vector<std::string>::iterator itr = Options::Paths.begin(); itr != Options::Paths.end(); itr++)
  {
    ProcessFile((*itr).c_str());
  }

  return 0;
}
