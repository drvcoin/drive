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
#include <limits>
#include <json/json.h>
#include <thread>
#include <fstream>
#include <errno.h>

#include "ActionHandler.h"

#include <drive/common/BdTypes.h>
#include <drive/common/BdSession.h>
#include <drive/client/Cache.h>
#include <drive/client/Util.h>
#include <drive/client/VolumeManager.h>
#include <drive/client/Paths.h>

#include <cm256.h>
#include <gf256.h>
#if defined(_WIN32)
#include "devio.h"
#else
#include <ubd.h>
#include <unistd.h>
#endif

namespace dfs
{
  std::map<std::string, VolumeMeta *> ActionHandler::volumeInfo;
  std::map<std::string, bool> ActionHandler::nbdInfo;
  
  // ubd callbacks  
  static size_t xmp_read(void *buf, size_t size, size_t offset, void * context)
  {
#ifdef __APPLE__
    printf("xmp_read: offset=%lld size=%lld\n", offset, size);
    Volume * volume = static_cast<Volume *>(context);
    if (offset + size > volume->BlockCount() * volume->DataCount() * volume->BlockSize())
    {
      return EINVAL;
    }
#elif _WIN32
    return ((Volume*)context)->ReadDecrypt(buf, size, offset) ? size : 0;
#else
    return ((Volume*)context)->ReadDecrypt(buf, size, offset) ? 0 : -1;
#endif
  }

  static size_t xmp_write(const void *buf, size_t size, size_t offset, void * context)
  {
#ifdef __APPLE__
    printf("xmp_write: offset=%lld size=%lld\n", offset, size);
    Volume * volume = static_cast<Volume *>(context);
    if (offset + size > volume->BlockCount() * volume->DataCount() * volume->BlockSize())
    {
      return EINVAL;
    }
#elif _WIN32
    return ((Volume*)context)->WriteEncrypt(buf, size, offset) ? size : 0;
#else
    return ((Volume*)context)->WriteEncrypt(buf, size, offset) ? 0 : -1;
#endif
  }

  static void xmp_disc(void * context)
  {
    (void)(context);
  }

  static void xmp_cleanup(void * context)
  {
    if(context)
    {
      delete ((Volume*)context);
      printf("Deleteing volume from xmp_cleanup\n");
    }
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

  void ActionHandler::Unmount(const std::string &nbdPath, bool matchAll=false)
  {
#if defined(_WIN32)
    printf("Unmounting %s\n", nbdPath.c_str());
    std::string cmd = "imdisk -D -m " + nbdPath;
    system(cmd.c_str());
#else
    std::ifstream fp("/proc/mounts");
    std::string line,s1,s2;
    while(fp >> s1 >> s2 && std::getline(fp,line))
    {
      if((s1.compare(0,nbdPath.length(),nbdPath) == 0 && matchAll) 
        || s1 == nbdPath)
      {
        printf("Unmounting %s\n",s2.c_str());
        std::string cmd = "umount " + s2;
        system(cmd.c_str());
      }
    }
    fp.close();
#endif
  }

  void ActionHandler::Cleanup()
  {
    printf("Unmounting volumes...\n");
#if !defined(_WIN32)
    ActionHandler::Unmount("/dev/nbd",true);
#endif

    for(auto &it : volumeInfo)
    {
      printf("Cleaning %s...\n",it.first.c_str());
#if defined(_WIN32)
      ActionHandler::Unmount(it.second->mountPath);
      drv_disconnect(it.first);
#else 
      ubd_disconnect(it.second->nbdPath.c_str());
#endif
      delete it.second;
    }
  }

  // returns {0: fail, 1: success, 2: already binded}
  int ActionHandler::BindVolume(const std::string &name, const std::string &path)
  {
    std::string nbdPath = ActionHandler::GetNextNBD();
#if !defined(_WIN32)
    if(nbdPath == "") 
    {
      printf("No available block devices.\n");
      return 0;
    }
#endif

    if(volumeInfo.find(name) != volumeInfo.end())
    {
      printf("Volume is binded to '%s'.\n",volumeInfo[name]->nbdPath.c_str());
      if(!path.empty())
      {
        volumeInfo[name]->mountPath = path;
      }
#if defined(_WIN32)
      if (!drv_task_active(name))
      {
        ActionHandler::UnbindVolume(name);
      }
      else
      {
        return 2;
      }
#endif
      return 2;
    }

#if 0
    uint64_t blockCount = 256;//*1024*1024ul;
    size_t blockSize = 1*1024*1024;
    std::string volumeId = path;
#endif

    auto volume = VolumeManager::LoadVolume(name);

    if(volume == nullptr)
    {
      printf("Failed to load volume '%s'\n",name.c_str());
      return 0;
    }

    // Cache size set to 100MB / (blockSize * (dataCount + codeCount)), flushing every 10 seconds
    std::string cacheDir = GetWorkingDir() + SLASH + name + SLASH + "cache";
#ifndef __APPLE__
    volume->EnableCache(std::make_unique<dfs::Cache>(cacheDir, volume.get(), 200, 10));
 #endif
    volume->EnableCache(std::make_unique<dfs::Cache>(cacheDir, volume.get(), 200, 10));
    
#if defined(_WIN32)
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
#else
    printf("Processing: %s\n", nbdPath.c_str());

    static struct ubd_operations ops = {
      .read = xmp_read,
      .write = xmp_write,
      .disc = xmp_disc,
      .flush = xmp_flush,
      .trim = xmp_trim,
      .cleanup = xmp_cleanup
    };
    
    Volume *pVolume = volume.release();
    ubd_register(nbdPath.c_str(), pVolume->DataSize(), pVolume->GetTimeout(), &ops, (void *)pVolume);
    
    int counter = 10;
    while(!nbd_ready(nbdPath.c_str()) && counter--)
    {
      sleep(1);
    }

    if(counter == 0)
    {
      return 0;
    }
#endif
    VolumeMeta *meta = new VolumeMeta;
    meta->volumeName = name;
    meta->nbdPath = nbdPath;
    meta->mountPath = path;
    meta->isMounted = meta->isFormatted = false;

    volumeInfo[name] = meta;
    nbdInfo[nbdPath] = true;

    return 1;
  }

  
  // returns {0: fail, 1: success, 2: already binded}
  int ActionHandler::UnbindVolume(const std::string &name)
  {
    auto it = volumeInfo.find(name);
    if(it != volumeInfo.end())
    {
      printf("Unbinding volume: %s\n",name.c_str());
#if defined(_WIN32)
      ActionHandler::Unmount(it->second->mountPath);
      drv_disconnect(name);
#else
      ActionHandler::Unmount(it->second->nbdPath);
      ubd_disconnect(it->second->nbdPath.c_str());
#endif
      nbdInfo[it->second->nbdPath] = false;
      delete it->second;
      volumeInfo.erase(it);
    }
    return 1;
  }
  
  std::string ActionHandler::GetNextNBD()
  {
    for(auto it : ActionHandler::nbdInfo)
    {
      if(!it.second)
        return it.first;
    }
    return "";
  }
}
