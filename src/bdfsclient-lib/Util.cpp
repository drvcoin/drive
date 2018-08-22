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

#include "Util.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <cerrno>
#include <memory>
#include <sstream>

#ifndef _WIN32
#include <unistd.h>

uint64_t htonll(uint64_t val)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return (((uint64_t) htonl(val)) << 32) + htonl(val >> 32);
#else
  return val;
#endif
}

uint64_t ntohll(uint64_t val)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return (((uint64_t) ntohl(val)) << 32) + ntohl(val >> 32);
#else
  return val;
#endif
}


bool nbd_ready(const char* devname, bool do_print) {
  char buf[256];
  char* p;
  int fd;
  int len;

  if( (p=strrchr((char*)devname, '/')) ) {
    devname=p+1;
  }
  if((p=strchr((char*)devname, 'p'))) {
    /* We can't do checks on partitions. */
    *p='\0';
  }
  snprintf(buf, 256, "/sys/block/%s/pid", devname);
  if((fd=open(buf, O_RDONLY))<0) {
    if(errno==ENOENT) {
      return false;
    } else {
      return false;
    }
  }
  len=read(fd, buf, 256);
  if(len < 0) {
    perror("could not read from server");
    close(fd);
    return false;
  }
  buf[(len < 256) ? len : 255]='\0';
  if(do_print) printf("%s\n", buf);
  close(fd);
  return true;
}

std::string execCmd(std::string cmd)
{
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get()) && !ferror(pipe.get()))
  {
    if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
    {
      result += buffer.data();
    }
  }
  return result;
}
#endif
