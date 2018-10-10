/*
Server end for ImDisk Virtual Disk Driver proxy operation.

Copyright (C) 2005-2015 Olof Lagerkvist.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
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
#include <Windows.h>
#include <stdint.h>
#include <thread>
#include "devio_types.h"
#ifdef __cplusplus
extern "C" {
#endif

  typedef safeio_ssize_t(__cdecl *dllread_proc)(void *handle,
    void *buf,
    safeio_size_t size,
    off_t_64 offset);

  typedef safeio_ssize_t(__cdecl *dllwrite_proc)(void *handle,
    void *buf,
    safeio_size_t size,
    off_t_64 offset);

  typedef int(__cdecl *dllclose_proc)(void *handle);

  typedef void * (__cdecl *dllopen_proc)(const char *file,
    int read_only,
    dllread_proc *dllread,
    dllwrite_proc *dllwrite,
    dllclose_proc *dllclose,
    off_t_64 *size);

#ifdef __cplusplus

  struct drv_operations
  {
    size_t(*read)(void * buffer, size_t size, size_t offset, void * context);
    size_t(*write)(const void * buffer, size_t size, size_t offset, void * context);
    void(*cleanup)(void * context);
  };

  bool drv_register(const char * path, size_t size, struct drv_operations * operations, void * context);
  bool drv_disconnect(const std::string &name);
  bool drv_task_active(const std::string &name);
  inline std::string drv_imdisk_shm_name(const std::string &name) { return name + "_shm"; }
}
#endif