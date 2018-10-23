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

#include <string>

#define return_false_if(condition) \
  if (condition) { return false; }

#define return_false_if_msg(condition, ...) \
  if (condition) { printf(__VA_ARGS__); return false; }

#if !defined(__APPLE__)
uint64_t htonll(uint64_t val);
uint64_t ntohll(uint64_t val);
#endif

#if defined(_WIN32)
#define S_IRWXU 0000700 /* RWX mask for owner */
#define S_IRWXG 0000070 /* RWX mask for group */
#define S_IRWXO 0000007 /* RWX mask for other */
#define S_IROTH 0000004 /* R for other */
#define S_IWOTH 0000002 /* W for other */
#define S_IXOTH 0000001 /* X for other */
#define unlink _unlink

void mkdir(const char* path, int flags = 0);

#else // Unix/Mac

#include <arpa/inet.h>
bool nbd_ready(const char* devname, bool do_print = false);
std::string execCmd(std::string cmd);
#endif
