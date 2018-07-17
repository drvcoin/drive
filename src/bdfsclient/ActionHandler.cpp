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


#include "VolumeManager.cpp"
#include <stdio.h>
#include <sys/stat.h>
#include <cmath>
#include <limits>
#include <json/json.h>
#include <thread>
#include <fstream>

#include "ActionHandler.h"
#include "BdTypes.h"
#include "BdSession.h"
#include "Cache.h"
#include "Util.h"

#include "cm256.h"
#include "gf256.h"
#include "devio.h"

namespace dfs
{
  std::unordered_map<std::string, std::string> ActionHandler::volumeInfo;

  // drv callbacks
  static size_t xmp_read(void *buf, size_t size, size_t offset, void * context)
  {
    auto r = ((Volume*)context)->ReadDecrypt(buf, size, offset) ? size : 0;
    //printf("read size:%d, offset:%d, ret:%d\n", size, offset, r);
    return r;
  }

  static size_t xmp_write(const void *buf, size_t size, size_t offset, void * context)
  {
    auto r = ((Volume*)context)->WriteEncrypt(buf, size, offset) ? size : 0;
    //printf("write size:%d, offset:%d, ret:%d", size, offset,r);
    return r;
  }


  static void xmp_cleanup(void * context)
  {
    if (context)
    {
      delete ((Volume*)context);
      printf("Deleteing volume from xmp_cleanup\n");
    }
  }

  void ActionHandler::Unmount(const std::string &path, bool matchAll = false)
  {
    printf("Unmounting %s\n", path.c_str());
    std::string cmd = "imdisk -D -m " + path;
    system(cmd.c_str());
  }

  void ActionHandler::Cleanup()
  {
    printf("Unmounting volumes...\n");
    for (auto &it : volumeInfo)
    {
      printf("Cleaning %s...\n", it.second.c_str());
      ActionHandler::Unmount(it.second.c_str());
      drv_disconnect(it.first.c_str());
    }
  }

  // returns {0: fail, 1: success, 2: already binded}
  int ActionHandler::BindVolume(const std::string &name, const std::string &path)
  {
    if (volumeInfo.find(name) != volumeInfo.end())
    {
      printf("Volume is binded to '%s'.\n", volumeInfo[name].c_str());
      if (!path.empty())
      {
        volumeInfo[name] = path;
      }
      if (!drv_task_active(name))
      {
        ActionHandler::UnbindVolume(name);
      }
      else
      {
        return 2;
      }
    }

    auto volume = VolumeManager::LoadVolume(name);

    if (volume == nullptr)
    {
      printf("Failed to load volume '%s'\n", name.c_str());
      return 0;
    }

    // Cache size set to 100MB / (blockSize * (dataCount + codeCount)), flushing every 10 seconds
    volume->EnableCache(std::make_unique<dfs::Cache>("cache", volume.get(), 200, 10));


    static struct drv_operations ops;
    ops.read = xmp_read;
    ops.write = xmp_write;
    ops.cleanup = xmp_cleanup;

    Volume *pVolume = volume.release();
    drv_register(name.c_str(), pVolume->DataSize(), &ops, (void *)pVolume);

    int counter = 10;
    while (!drv_task_active(name) && counter--)
    {
      Sleep(1000);
    }

    volumeInfo[name] = path;

    return 1;
  }


  // returns {0: fail, 1: success, 2: already binded}
  int ActionHandler::UnbindVolume(const std::string &name)
  {
    auto it = volumeInfo.find(name);
    if (it != volumeInfo.end())
    {
      printf("Unbinding volume: %s\n", name.c_str());
      ActionHandler::Unmount(it->second);
      drv_disconnect(name);
      volumeInfo.erase(it);
    }
    return 1;
  }
}
