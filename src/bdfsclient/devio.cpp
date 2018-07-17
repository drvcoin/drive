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

#define WIN32_LEAN_AND_MEAN
#define __USE_UNIX98

#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

#ifdef _WIN32

#pragma warning(disable: 4201)

#include <windows.h>
#include <winsock2.h>
#include <winioctl.h>
#include <io.h>
#include <unordered_map>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")
#include "inc/imdproxy.h"
#include "devio_types.h"
#include "safeio.h"
#include "devio.h"
#endif


#ifndef O_DIRECT
#define O_DIRECT 0
#endif

#ifndef O_FSYNC
#define O_FSYNC 0
#endif

#define DEF_BUFFER_SIZE (2 << 20)

#define DEF_REQUIRED_ALIGNMENT 1

#ifdef DEBUG
#define dbglog(x) syslog x
#else
#define dbglog(x)
#endif

__inline
void
syslog(FILE *Stream, LPCSTR Message, ...)
{
  va_list param_list;
  LPSTR MsgBuf = NULL;

  va_start(param_list, Message);

  if (strstr(Message, "%m") != NULL)
  {
    DWORD winerrno = GetLastError();

    if (winerrno == NO_ERROR)
      winerrno = 10000 + errno;

    if (FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK |
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, winerrno, 0, (LPSTR)&MsgBuf, 0, NULL))
    {
      CharToOemA(MsgBuf, MsgBuf);
    }
    else
    {
      MsgBuf = NULL;
    }
  }

  if (MsgBuf != NULL)
  {
    vfprintf(Stream, Message, param_list);
    fprintf(Stream, "%s\n", MsgBuf);

    LocalFree(MsgBuf);
  }
  else
    vfprintf(Stream, Message, param_list);

  fflush(Stream);
  return;
}

#define LOG_ERR       stderr

#define OBJNAME_SIZE  260

int64_t GetBigEndian64(int8_t *storage)
{
  int i;
  int64_t number = 0;
  for (i = 0; i < sizeof(int64_t); i++)
  {
    number |= (int64_t)storage[i] << ((sizeof(int64_t) - i - 1) << 3);
  }
  return number;
}

struct _VHD_INFO
{
  struct _VHD_FOOTER
  {
    uint8_t Cookie[8];
    uint32_t Features;
    uint32_t FileFormatVersion;
    int64_t DataOffset;
    uint32_t TimeStamp;
    uint32_t CreatorApplication;
    uint32_t CreatorVersion;
    uint32_t CreatorHostOS;
    int64_t OriginalSize;
    int8_t CurrentSize[sizeof(int64_t)];
    uint32_t DiskGeometry;
    uint32_t DiskType;
    uint32_t Checksum;
    uint8_t UniqueID[16];
    uint8_t SavedState;
    uint8_t Padding[427];
  } Footer;

  struct _VHD_HEADER
  {
    uint8_t Cookie[8];
    int64_t DataOffset;
    int8_t TableOffset[sizeof(int64_t)];
    uint32_t HeaderVersion;
    uint32_t MaxTableEntries;
    uint32_t BlockSize;
    uint32_t Checksum;
    uint8_t ParentUniqueID[16];
    uint32_t ParentTimeStamp;
    uint32_t Reserved1;
    uint16_t ParentName[256];
    struct _VHD_PARENT_LOCATOR
    {
      uint32_t PlatformCode;
      uint32_t PlatformDataSpace;
      uint32_t PlatformDataLength;
      uint32_t Reserved1;
      int64_t PlatformDataOffset;
    } ParentLocator[8];
    uint8_t Padding[256];
  } Header;

};

struct _DEVIO_INFO {
  _VHD_INFO vhd_info = { { { 0 } } };
  HANDLE shm_server_mutex = NULL;
  HANDLE shm_request_event = NULL;
  HANDLE shm_response_event = NULL;


  int fd = -1;
  void *libhandle = NULL;
  SOCKET sd = INVALID_SOCKET;
  int shm_mode = 0;
  char *shm_readptr = 0;
  char *shm_writeptr = 0;
  char *shm_view = NULL;
  char *buf = NULL;
  char *buf2 = NULL;
  safeio_size_t buffer_size = DEF_BUFFER_SIZE;
  off_t_64 offset = 0;
  IMDPROXY_INFO_RESP devio_info = { 0 };
  char vhd_mode = 0;
  char auto_vhd_detect = 1;


  safeio_size_t block_size = 0;
  safeio_size_t sector_size = 512;
  off_t_64 table_offset = 0;
  int16_t block_shift = 0;
  int16_t sector_shift = 0;
  off_t_64 current_size = 0;

  HANDLE hFileMap = NULL;
  HANDLE hShared = NULL;

  // drv_ops
  drv_operations *ops = NULL;
  void *context = NULL;
  bool active_worker = false;
  std::thread worker_task;
};

std::unordered_map<std::string, std::shared_ptr<_DEVIO_INFO>> devio_map;

safeio_ssize_t
physical_read(void *io_ptr, safeio_size_t size, off_t_64 offset, std::shared_ptr<_DEVIO_INFO> o)
{
  return o->ops->read(io_ptr, size, offset, o->context);
}

safeio_ssize_t
physical_write(void *io_ptr, safeio_size_t size, off_t_64 offset, std::shared_ptr<_DEVIO_INFO> o)
{
  return o->ops->write(io_ptr, size, offset, o->context);
}

int
physical_close(std::shared_ptr<_DEVIO_INFO> o)
{
  o->ops->cleanup(o->context);
  return true;
}

#ifdef _WIN32

int
shm_read(void *io_ptr, safeio_size_t size, std::shared_ptr<_DEVIO_INFO> o)
{
  if (io_ptr == o->buf)
    if (size <= o->buffer_size)
      return 1;
    else
      return 0;

  if (o->shm_readptr == NULL)
    o->shm_readptr = o->shm_view;

  if ((long long)size > (long long)(o->buf - o->shm_readptr))
    return 0;

  memcpy(io_ptr, o->shm_readptr, size);
  o->shm_readptr = o->shm_readptr + size;

  return 1;
}

int
shm_write(const void *io_ptr, safeio_size_t size, std::shared_ptr<_DEVIO_INFO> o)
{
  if (io_ptr == o->buf)
    if (size <= o->buffer_size)
      return 1;
    else
      return 0;

  if (o->shm_writeptr == NULL)
    o->shm_writeptr = o->shm_view;

  if ((long long)size > (long long)(o->buf - o->shm_writeptr))
    return 0;

  memcpy(o->shm_writeptr, io_ptr, size);
  o->shm_writeptr = o->shm_writeptr + size;

  return 1;
}

int
shm_flush(std::shared_ptr<_DEVIO_INFO> o)
{
  o->shm_readptr = NULL;
  o->shm_writeptr = NULL;

  if (!SetEvent(o->shm_response_event))
  {
    syslog(LOG_ERR, "SetEvent() failed: %m\n");
    return 0;
  }

  if (WaitForSingleObject(o->shm_request_event, INFINITE) != WAIT_OBJECT_0)
    return 0;

  return 1;
}

int
comm_flush(std::shared_ptr<_DEVIO_INFO> o)
{
  if (o->shm_mode)
    return shm_flush(o);
  else
    return 1;
}

int
comm_read(void *io_ptr, safeio_size_t size, std::shared_ptr<_DEVIO_INFO> o)
{
  if (o->shm_mode)
    return shm_read(io_ptr, size, o);
  else
    return safe_read(o->sd, io_ptr, size);
}

int
comm_write(const void *io_ptr, safeio_size_t size, std::shared_ptr<_DEVIO_INFO> o)
{
  if (o->shm_mode)
    return shm_write(io_ptr, size, o);
  else
    return safe_write(o->sd, io_ptr, size);
}

int
send_info(std::shared_ptr<_DEVIO_INFO> o)
{
  if (!comm_write(&o->devio_info, sizeof o->devio_info, o))
    return 0;

  if (!comm_flush(o))
  {
    syslog(LOG_ERR, "Error flushing comm data: %m\n");
    return 0;
  }

  return 1;
}

safeio_ssize_t
vhd_read(char *io_ptr, safeio_size_t size, off_t_64 offset, std::shared_ptr<_DEVIO_INFO> o)
{
  off_t_64 block_number;
  off_t_64 data_offset;
  safeio_size_t in_block_offset;
  uint32_t block_offset;
  safeio_size_t first_size = size;
  off_t_64 second_offset = 0;
  safeio_size_t second_size = 0;
  safeio_ssize_t readdone;

  dbglog((LOG_ERR, "vhd_read: Request " SLL_FMT " bytes at " SLL_FMT ".\n",
    (off_t_64)size, (off_t_64)offset));

  if (offset + size > o->current_size)
    return 0;

  block_number = (safeio_size_t)(offset >> o->block_shift);
  data_offset = o->table_offset + (block_number << 2);
  in_block_offset = (safeio_size_t)(offset & (o->block_size - 1));
  if (first_size + in_block_offset > o->block_size)
  {
    first_size = o->block_size - in_block_offset;
    second_size = size - first_size;
    second_offset = offset + first_size;
  }

  readdone = physical_read(&block_offset, sizeof(block_offset), data_offset, o);
  if (readdone != sizeof(block_offset))
  {
    syslog(LOG_ERR, "vhd_read: Error reading block table: %m\n");

    if (errno == 0)
      errno = E2BIG;

    return (safeio_ssize_t)-1;
  }

  memset(io_ptr, 0, size);

  if (block_offset == 0xFFFFFFFF)
    readdone = first_size;
  else
  {
    block_offset = ntohl(block_offset);

    data_offset =
      (((off_t_64)block_offset) << o->sector_shift) + o->sector_size +
      in_block_offset;

    readdone = physical_read(io_ptr, (safeio_size_t)first_size, data_offset, o);
    if (readdone == -1)
      return (safeio_ssize_t)-1;
  }

  if (second_size > 0)
  {
    safeio_size_t second_read =
      vhd_read(io_ptr + first_size, second_size, second_offset, o);

    if (second_read == -1)
      return (safeio_ssize_t)-1;

    readdone += second_read;
  }

  return readdone;
}

safeio_ssize_t
vhd_write(char *io_ptr, safeio_size_t size, off_t_64 offset, std::shared_ptr<_DEVIO_INFO> o)
{
  off_t_64 block_number;
  off_t_64 data_offset;
  safeio_size_t in_block_offset;
  uint32_t block_offset;
  safeio_size_t first_size = size;
  off_t_64 second_offset = 0;
  safeio_size_t second_size = 0;
  safeio_ssize_t readdone;
  safeio_ssize_t writedone;
  off_t_64 bitmap_offset;
  safeio_size_t bitmap_datasize;
  safeio_size_t first_size_nqwords;

  dbglog((LOG_ERR, "vhd_write: Request " SLL_FMT " bytes at " SLL_FMT ".\n",
    (off_t_64)size, (off_t_64)offset));

  if (offset + size > o->current_size)
    return 0;

  block_number = offset >> o->block_shift;
  data_offset = o->table_offset + (block_number << 2);
  in_block_offset = (safeio_size_t)(offset & (o->block_size - 1));
  if (first_size + in_block_offset > o->block_size)
  {
    first_size = o->block_size - in_block_offset;
    second_size = size - first_size;
    second_offset = offset + first_size;
  }
  first_size_nqwords = (first_size + 7) >> 3;

  readdone = physical_read(&block_offset, sizeof(block_offset), data_offset, o);
  if (readdone != sizeof(block_offset))
  {
    syslog(LOG_ERR, "vhd_write: Error reading block table: %m\n");

    if (errno == 0)
      errno = E2BIG;

    return (safeio_ssize_t)-1;
  }

  // Alocate a new block if not already defined
  if (block_offset == 0xFFFFFFFF)
  {
    off_t_64 block_offset_bytes;
    char *new_block_buf;
    long long *buf_ptr;

    // First check if new block is all zeroes, in that case don't allocate
    // a new block in the vhd file
    for (buf_ptr = (long long*)io_ptr;
      (buf_ptr < (long long*)io_ptr + first_size_nqwords) ? 0 :
      (*buf_ptr == 0);
      buf_ptr++);
    if (buf_ptr >= (long long*)io_ptr + first_size_nqwords)
    {
      dbglog((LOG_ERR, "vhd_write: New empty block not added to vhd file "
        "backing " SLL_FMT " bytes at " SLL_FMT ".\n",
        (off_t_64)first_size, (off_t_64)offset));

      writedone = first_size;

      if (second_size > 0)
      {
        safeio_size_t second_write =
          vhd_write(io_ptr + first_size, second_size, second_offset, o);

        if (second_write == -1)
          return (safeio_ssize_t)-1;

        writedone += second_write;
      }

      return writedone;
    }

    dbglog((LOG_ERR, "vhd_write: Adding new block to vhd file backing "
      SLL_FMT " bytes at " SLL_FMT ".\n",
      (off_t_64)first_size, (off_t_64)offset));

    new_block_buf = (char *)
      malloc(o->sector_size + o->block_size + sizeof(o->vhd_info.Footer));
    if (new_block_buf == NULL)
    {
      syslog(LOG_ERR, "vhd_write: Error allocating memory buffer for new "
        "block: %m\n");

      return (safeio_ssize_t)-1;
    }

    // New block is placed where the footer currently is
    block_offset_bytes =
      _lseeki64(o->fd, -(off_t_64) sizeof(o->vhd_info.Footer), SEEK_END);
    if (block_offset_bytes == -1)
    {
      syslog(LOG_ERR, "vhd_write: Error moving file pointer to last "
        "block: %m\n");

      free(new_block_buf);
      return (safeio_ssize_t)-1;
    }

    // Store pointer to new block start sector in BAT
    block_offset = htonl((uint32_t)(block_offset_bytes >> o->sector_shift));
    readdone =
      physical_write(&block_offset, sizeof(block_offset), data_offset, o);
    if (readdone != sizeof(block_offset))
    {
      syslog(LOG_ERR, "vhd_write: Error updating BAT: %m\n");

      free(new_block_buf);

      if (errno == 0)
        errno = E2BIG;

      return (safeio_ssize_t)-1;
    }

    // Initialize new block with zeroes followed by the new footer
    memset(new_block_buf, 0, o->sector_size + o->block_size);
    memcpy(new_block_buf + o->sector_size + o->block_size, &o->vhd_info.Footer,
      sizeof(o->vhd_info.Footer));

    readdone =
      physical_write(new_block_buf,
        o->sector_size + o->block_size + sizeof(o->vhd_info.Footer),
        block_offset_bytes, o);
    if (readdone != (safeio_ssize_t)(o->sector_size + o->block_size) +
      (safeio_ssize_t)sizeof(o->vhd_info.Footer))
    {
      syslog(LOG_ERR, "vhd_write: Error writing new block: %m\n");

      free(new_block_buf);

      if (errno == 0)
        errno = E2BIG;

      return (safeio_ssize_t)-1;
    }

    free(new_block_buf);
  }

  // Calculate where actual data should be written
  block_offset = ntohl(block_offset);
  data_offset = (((off_t_64)block_offset) << o->sector_shift) + o->sector_size +
    in_block_offset;

  // Write data
  writedone = physical_write(io_ptr, (safeio_size_t)first_size, data_offset, o);
  if (writedone == -1)
    return (safeio_ssize_t)-1;

  // Calculate where and how many bytes in allocation bitmap we need to update
  bitmap_offset = ((off_t_64)block_offset << o->sector_shift) +
    (in_block_offset >> o->sector_shift >> 3);

  bitmap_datasize =
    (((first_size + o->sector_size - 1) >> o->sector_shift) + 7) >> 3;

  // Set bits as 'allocated'
  memset(o->buf2, 0xFF, bitmap_datasize);

  // Update allocation bitmap
  readdone = physical_write(o->buf2, bitmap_datasize, bitmap_offset, o);
  if (readdone != (safeio_ssize_t)bitmap_datasize)
  {
    syslog(LOG_ERR, "vhd_write: Error updating block bitmap: %m\n");

    if (errno == 0)
      errno = E2BIG;

    return (safeio_ssize_t)-1;
  }

  if (second_size > 0)
  {
    safeio_size_t second_write =
      vhd_write(io_ptr + first_size, second_size, second_offset, o);

    if (second_write == -1)
      return (safeio_ssize_t)-1;

    writedone += second_write;
  }

  return writedone;
}

safeio_ssize_t
logical_read(char *io_ptr, safeio_size_t size, off_t_64 offset, std::shared_ptr<_DEVIO_INFO> o)
{
  if (o->vhd_mode)
    return vhd_read(io_ptr, size, offset, o);
  else
    return physical_read(io_ptr, size, offset, o);
}

safeio_ssize_t
logical_write(char *io_ptr, safeio_size_t size, off_t_64 offset, std::shared_ptr<_DEVIO_INFO> o)
{
  if (o->vhd_mode)
    return vhd_write(io_ptr, size, offset, o);
  else
    return physical_write(io_ptr, size, offset, o);
}

int
read_data(std::shared_ptr<_DEVIO_INFO> o)
{
  IMDPROXY_READ_REQ req_block = { 0 };
  IMDPROXY_READ_RESP resp_block = { 0 };
  safeio_size_t size;
  safeio_ssize_t readdone;

  if (!comm_read(&req_block.offset,
    sizeof(req_block) - sizeof(req_block.request_code), o))
  {
    syslog(LOG_ERR, "Error reading request header.\n");
    return 0;
  }

  size = (safeio_size_t)
    (req_block.length < o->buffer_size ? req_block.length : o->buffer_size);

  dbglog((LOG_ERR, "read request " ULL_FMT " bytes at " ULL_FMT " + "
    ULL_FMT " = " ULL_FMT ".\n",
    req_block.length, req_block.offset, offset,
    req_block.offset + offset));

  memset(o->buf, 0, size);

  readdone =
    logical_read(o->buf, (safeio_size_t)size, (off_t_64)(o->offset + req_block.offset), o);
  if (readdone == -1)
  {
    resp_block.errorno = errno;
    resp_block.length = 0;
    syslog(LOG_ERR, "Device read: %m\n");
  }
  else
  {
    resp_block.errorno = 0;
    resp_block.length = size;

    if (req_block.length != readdone)
    {
      syslog(LOG_ERR,
        "Partial read at " SLL_FMT ": Got " SLL_FMT ", req " ULL_FMT ".\n",
        (int64_t)(o->offset + req_block.offset), (int64_t)readdone, req_block.length);
    }
  }

  dbglog((LOG_ERR, "read done reporting/sending " ULL_FMT " bytes.\n",
    resp_block.length));

  if (!comm_write(&resp_block, sizeof(resp_block), o))
  {
    syslog(LOG_ERR, "Warning: I/O stream inconsistency.\n");
    return 0;
  }

  if (resp_block.errorno == 0)
    if (!comm_write(o->buf, (safeio_size_t)resp_block.length, o))
    {
      syslog(LOG_ERR, "Error sending read response to caller.\n");
      return 0;
    }

  if (!comm_flush(o))
  {
    syslog(LOG_ERR, "Error flushing comm data: %m\n");
    return 0;
  }

  return 1;
}

int
write_data(std::shared_ptr<_DEVIO_INFO> o)
{
  IMDPROXY_WRITE_REQ req_block = { 0 };
  IMDPROXY_WRITE_RESP resp_block = { 0 };

  if (!comm_read(&req_block.offset,
    sizeof(req_block) - sizeof(req_block.request_code), o))
    return 0;

  dbglog((LOG_ERR, "write request " ULL_FMT " bytes at " ULL_FMT " + "
    ULL_FMT " = " ULL_FMT ".\n",
    req_block.length, req_block.offset, offset,
    req_block.offset + offset));

  if (req_block.length > o->buffer_size)
  {
    syslog(LOG_ERR, "Too big block write requested: %u bytes, buffer: %u bytes.\n",
      (int)req_block.length, o->buffer_size);
    return 0;
  }

  if (!comm_read(o->buf, (safeio_size_t)req_block.length, o))
  {
    syslog(LOG_ERR, "Warning: I/O stream inconsistency.\n");

    return 0;
  }

  if (o->devio_info.flags & IMDPROXY_FLAG_RO)
  {
    resp_block.errorno = EBADF;
    resp_block.length = 0;
    syslog(LOG_ERR, "Device write attempt on read-only device.\n");
  }
  else
  {
    safeio_ssize_t writedone = logical_write(o->buf, (safeio_size_t)req_block.length,
      (off_t_64)(o->offset + req_block.offset), o);
    if (writedone == -1)
    {
      resp_block.errorno = errno;
      resp_block.length = writedone;
#ifdef _WIN32
      perror("Device write");
#else
      syslog(LOG_ERR, "Device write: %m\n");
#endif
    }
    else
    {
      resp_block.errorno = 0;
      resp_block.length = writedone;
    }

    if (req_block.length != resp_block.length)
    {
      if (writedone < 0)
      {
        syslog(LOG_ERR, "Write error (code " ULL_FMT ") at " ULL_FMT ": Req " ULL_FMT ".\n",
          resp_block.errorno, o->offset + req_block.offset, req_block.length);
      }
      else
      {
        syslog(LOG_ERR, "Partial write at " ULL_FMT ": Got " ULL_FMT ", req " ULL_FMT ".\n",
          resp_block.errorno, o->offset + req_block.offset, req_block.length);
      }
    }

    dbglog((LOG_ERR, "write done reporting/sending " ULL_FMT " bytes.\n",
      resp_block.length));
  }

  if (!comm_write(&resp_block, sizeof resp_block, o))
  {
    syslog(LOG_ERR, "Error sending write response to caller.\n");

    return 0;
  }

  if (!comm_flush(o))
  {
    syslog(LOG_ERR, "Error flushing comm data: %m\n");
    return 0;
  }

  return 1;
}


int
do_comm_shm(const char *comm_device, std::shared_ptr<_DEVIO_INFO> o)
{
  MEMORY_BASIC_INFORMATION memory_info = { 0 };
  safeio_size_t detected_buffer_size = 0;
  ULARGE_INTEGER map_size = { 0 };
  char *objname = (char*)malloc(OBJNAME_SIZE);
  char *namespace_prefix;
  o->hShared = CreateFile("\\\\?\\Global", 0, FILE_SHARE_READ, NULL,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if ((o->hShared == INVALID_HANDLE_VALUE) & (GetLastError() == ERROR_FILE_NOT_FOUND))
    namespace_prefix = (char*)"";
  else
    namespace_prefix = (char*)"Global\\";

  if (o->hShared != INVALID_HANDLE_VALUE)
    CloseHandle(o->hShared);

  if (objname == NULL)
  {
    syslog(LOG_ERR, "Memory allocation failed: %m\n");
    return -1;
  }

  puts("Shared memory operation.");

  _snprintf(objname, OBJNAME_SIZE,
    "%s%s", namespace_prefix, comm_device);
  objname[OBJNAME_SIZE - 1] = 0;

  map_size.QuadPart = o->buffer_size + IMDPROXY_HEADER_SIZE;

  o->hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE | SEC_COMMIT,
    map_size.HighPart,
    map_size.LowPart,
    objname);

  if (o->hFileMap == NULL)
  {
    syslog(LOG_ERR, "CreateFileMapping() failed: %m\n");
    return 2;
  }

  if (GetLastError() == ERROR_ALREADY_EXISTS)
  {
    syslog(LOG_ERR, "A service with this name is already running.\n");
    //return 2;
  }

  o->shm_view = (char*)MapViewOfFile(o->hFileMap, FILE_MAP_WRITE, 0, 0, 0);

  if (o->shm_view == NULL)
  {
    syslog(LOG_ERR, "MapViewOfFile() failed: %m\n");
    return 2;
  }

  o->buf = o->shm_view + IMDPROXY_HEADER_SIZE;

  if (!VirtualQuery(o->shm_view, &memory_info, sizeof(memory_info)))
  {
    syslog(LOG_ERR, "VirtualQuery() failed: %m\n");
    return 2;
  }

  detected_buffer_size = (safeio_size_t)
    (memory_info.RegionSize - IMDPROXY_HEADER_SIZE);

  if (o->buffer_size != detected_buffer_size)
  {
    o->buffer_size = detected_buffer_size;
    if (o->buf2 != NULL)
    {
      free(o->buf2);
      o->buf2 = (char*)malloc(o->buffer_size);
      if (o->buf2 == NULL)
      {
        syslog(LOG_ERR, "malloc() failed: %m\n");
        return 2;
      }
    }
  }

  _snprintf(objname, OBJNAME_SIZE,
    "%s%s_Server", namespace_prefix, comm_device);
  objname[OBJNAME_SIZE - 1] = 0;
  o->shm_server_mutex = CreateMutex(NULL, FALSE, objname);

  if (o->shm_server_mutex == NULL)
  {
    syslog(LOG_ERR, "CreateMutex() failed: %m\n");
    return 2;
  }

  if (WaitForSingleObject(o->shm_server_mutex, 0) != WAIT_OBJECT_0)
  {
    syslog(LOG_ERR, "A service with this name is already running (WaitForSingleObject).\n");
    return 2;
  }

  _snprintf(objname, OBJNAME_SIZE,
    "%s%s_Request", namespace_prefix, comm_device);
  objname[OBJNAME_SIZE - 1] = 0;
  o->shm_request_event = CreateEvent(NULL, FALSE, FALSE, objname);

  if (o->shm_request_event == NULL)
  {
    syslog(LOG_ERR, "CreateEvent() failed: %m\n");
    return 2;
  }

  _snprintf(objname, OBJNAME_SIZE,
    "%s%s_Response", namespace_prefix, comm_device);
  objname[OBJNAME_SIZE - 1] = 0;
  o->shm_response_event = CreateEvent(NULL, FALSE, FALSE, objname);

  if (o->shm_response_event == NULL)
  {
    syslog(LOG_ERR, "CreateEvent() failed: %m\n");
    return 2;
  }

  free(objname);
  objname = NULL;

  o->shm_mode = 1;
  o->active_worker = true;

  printf("Waiting for connection on object %s. Press Ctrl+C to cancel.\n",
    comm_device);

  if (WaitForSingleObject(o->shm_request_event, INFINITE) != WAIT_OBJECT_0)
  {
    syslog(LOG_ERR, "Wait failed: %m.\n");
    return 2;
  }

  printf("Connection on object %s.\n",
    comm_device);

  return 0;
}

void cleanup_comm(std::shared_ptr<_DEVIO_INFO> o)
{

  if (o->shm_request_event)
  {
    SetEvent(o->shm_request_event);
    CloseHandle(o->shm_request_event);
    o->shm_request_event = NULL;
  }

  if (o->shm_response_event)
  {
    SetEvent(o->shm_response_event);
    CloseHandle(o->shm_response_event);
    o->shm_response_event = NULL;
  }

  if (o->shm_server_mutex)
  {
    SetEvent(o->shm_server_mutex);
    CloseHandle(o->shm_server_mutex);
    o->shm_server_mutex = NULL;
  }

  if (o->shm_view)
  {
    UnmapViewOfFile(o->shm_view);
  }

  if (o->hFileMap)
  {
    CloseHandle(o->hFileMap);
    o->hFileMap = NULL;
  }

  if (o->hShared)
  {
    CloseHandle(o->hShared);
    o->hShared = NULL;
  }
}

bool drv_register(const char * path, size_t size, struct drv_operations * operations, void * context)
{
  if (operations == NULL || operations->read == NULL || operations->write == NULL)
  {
    printf("Invalid callbacks\n");
    return false;
  }

  std::string s_path(path);
  auto it = devio_map.find(s_path);
  if (it != devio_map.end()) {
    cleanup_comm(it->second);
    devio_map.erase(it);
  }

  auto o = std::make_shared<_DEVIO_INFO>();
  o->ops = operations;
  o->context = context;

  safeio_ssize_t readdone;
  int partition_number = 0;
  char mbr[512];
  int retval;
  char *comm_device = NULL;
  o->devio_info.file_size = size;

#ifdef _WIN32
  WSADATA wsadata;

  //SetUnhandledExceptionFilter(ExceptionFilter);

  WSAStartup(0x0101, &wsadata);
#endif

  printf("devio_info.file_size=%llu\n", o->devio_info.file_size);

  // Autodetect Microsoft .vhd files
  readdone = physical_read(&o->vhd_info, (safeio_size_t) sizeof(o->vhd_info), 0, o);

  if (o->auto_vhd_detect &&
    (readdone == sizeof(o->vhd_info)) &&
    (strncmp((char*)o->vhd_info.Header.Cookie, "cxsparse", 8) == 0) &&
    (strncmp((char*)o->vhd_info.Footer.Cookie, "conectix", 8) == 0) &&
    o->vhd_info.Footer.DiskType == 0x03000000UL)
  {
    void *geometry = &o->vhd_info.Footer.DiskGeometry;

    // VHD I/O uses a secondary buffer
    o->buf2 = (char*)malloc(o->buffer_size);
    if (o->buf2 == NULL)
    {
      syslog(LOG_ERR, "malloc() failed: %m\n");
      return false;
    }

    puts("Detected dynamically expanding Microsoft VHD image file format.");

    // Calculate vhd shifts
    o->current_size = GetBigEndian64(o->vhd_info.Footer.CurrentSize);
    o->table_offset = GetBigEndian64(o->vhd_info.Header.TableOffset);


    o->sector_size = 512;

    o->block_size = ntohl(o->vhd_info.Header.BlockSize);

    for (o->block_shift = 0;
      (o->block_shift < 64) &
      ((((safeio_size_t)1) << o->block_shift) != o->block_size);
      o->block_shift++);

    o->devio_info.file_size = o->current_size;

    o->vhd_mode = 1;

    printf("VHD block size: %u bytes. C/H/S geometry: %u/%u/%u.\n",
      (unsigned int)o->block_size,
      (unsigned int)ntohs(*(u_short*)geometry),
      (unsigned int)((u_char*)geometry)[2],
      (unsigned int)((u_char*)geometry)[3]);
  }

  for (o->sector_shift = 0;
    (o->sector_shift < 64) & ((((safeio_size_t)1) << o->sector_shift) !=
      o->sector_size); o->sector_shift++);

  partition_number = 1;

  if (o->devio_info.file_size == 0)
  {
    syslog(LOG_ERR, "Cannot determine size of image/partition.\n");
  }


  if (o->current_size == 0)
    o->current_size = o->devio_info.file_size;

  if (o->devio_info.file_size != 0)
  {
    printf("Image size used: " ULL_FMT " bytes, partition_num: %d\n", o->devio_info.file_size, partition_number);
  }

  if ((partition_number >= 1) & (partition_number <= 8))
  {
    if (logical_read(mbr, 512, 0, o) == -1)
      syslog(LOG_ERR, "Error reading device: %m\n");
    else if ((*(u_short*)(mbr + 0x01FE) == 0xAA55) &
      ((*(u_char*)(mbr + 0x01BE) & 0x7F) == 0) &
      ((*(u_char*)(mbr + 0x01CE) & 0x7F) == 0) &
      ((*(u_char*)(mbr + 0x01DE) & 0x7F) == 0) &
      ((*(u_char*)(mbr + 0x01EE) & 0x7F) == 0))
    {
      puts("Detected a master boot record at sector 0.");

      o->offset = 0;

      if ((partition_number >= 5) & (partition_number <= 8))
      {
        int i = 0;
        for (i = 0; i < 4; i++)
          switch (*(mbr + 512 - 66 + (i << 4) + 4))
          {
          case 0x05: case 0x0F:
            o->offset = (off_t_64)
              (*(u_int*)(mbr + 512 - 66 + (i << 4) + 8)) <<
              o->sector_shift;

            printf("Reading extended partition table at " SLL_FMT
              ".\n", (int64_t)o->offset);

            if (logical_read(mbr, 512, o->offset, o) == 512)
              if ((*(u_short*)(mbr + 0x01FE) == 0xAA55) &
                ((*(u_char*)(mbr + 0x01BE) & 0x7F) == 0) &
                ((*(u_char*)(mbr + 0x01CE) & 0x7F) == 0) &
                ((*(u_char*)(mbr + 0x01DE) & 0x7F) == 0) &
                ((*(u_char*)(mbr + 0x01EE) & 0x7F) == 0))
              {
                puts("Found valid extended partition table.");
                partition_number -= 4;
              }
          }

        if (partition_number > 4)
        {
          syslog(LOG_ERR,
            "No valid extended partition table found.\n");
          return 1;
        }
      }

      if ((partition_number >= 1) & (partition_number <= 4))
      {
        o->offset += (off_t_64)
          (*(u_int*)(mbr + 512 - 66 + ((partition_number - 1) << 4) + 8))
          << o->sector_shift;
        o->devio_info.file_size = (off_t_64)
          (*(u_int*)(mbr + 512 - 66 + ((partition_number - 1) << 4) + 12))
          << o->sector_shift;

        if ((o->devio_info.file_size == 0) |
          ((o->current_size != 0) &
          (o->offset + (off_t_64)o->devio_info.file_size > o->current_size)))
        {
          syslog(LOG_ERR,
            "Partition %i not found.\n", partition_number);
          return 1;
        }

        printf("Using partition %i.\n", partition_number);
      }
    }
    else
      printf("No master boot record detected. Using entire image.\n");
  }

  o->devio_info.req_alignment = DEF_REQUIRED_ALIGNMENT;

  printf("Total size: " SLL_FMT " bytes. Using " ULL_FMT " bytes from offset "
    SLL_FMT ".\n"
    "Required alignment: " ULL_FMT " bytes.\n"
    "Buffer size: " SIZ_FMT " bytes.\n",
    (int64_t)o->current_size,
    o->devio_info.file_size,
    (int64_t)o->offset,
    o->devio_info.req_alignment,
    o->buffer_size);

  // Create shared memory for imdisk communication

  devio_map[s_path] = std::move(o);

  devio_map[s_path]->worker_task = std::move(std::thread([s_path] {
    auto it = devio_map.find(s_path);
    if (it == devio_map.end())
      return 0;

    auto o = it->second;

    std::string str_comm = drv_imdisk_shm_name(s_path);
    int shmresult = do_comm_shm(str_comm.c_str(), o);
    if (shmresult != 0)
      return shmresult;

    ULONGLONG req = 0;
    while (o->active_worker)
    {
      if (!comm_read(&req, sizeof(req), o))
      {
        printf("Connection closed for %s\n", str_comm.c_str());
        o->active_worker = false;
      }

      switch (req)
      {
      case IMDPROXY_REQ_INFO:
        if (!send_info(o))
          o->active_worker = false;
        break;

      case IMDPROXY_REQ_READ:
        if (!read_data(o))
          o->active_worker = false;
        break;

      case IMDPROXY_REQ_WRITE:
        if (!write_data(o))
          o->active_worker = false;
        break;

      default:
        req = ENODEV;
        if (!comm_write(&req, sizeof req, o))
        {
          syslog(LOG_ERR, "comm_write failed.\n");
          o->active_worker = false;
        }
      }
    }
    physical_close(o);
    printf("Exitng worker for '%s'.\n", str_comm.c_str());
  }));

  devio_map[s_path]->worker_task.detach();

  return true;
}

bool drv_disconnect(const std::string &name)
{
  auto it = devio_map.find(name);
  if (it == devio_map.end())
    return false;

  auto o = it->second;
  o->active_worker = false;
  if (o->worker_task.joinable())
  {
    o->worker_task.join();
  }
  cleanup_comm(o);

  devio_map.erase(it);

  return true;
}

bool drv_task_active(const std::string &name)
{
  auto it = devio_map.find(name);
  if (it == devio_map.end())
    return false;
  return it->second->active_worker;
}

#endif