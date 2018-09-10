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
#include <unistd.h>
#include <json/json.h>
#include <thread>
#include <fstream>
#include <errno.h>

#include "ActionHandler.h"
#include "BdTypes.h"
#include "BdSession.h"
#include "Cache.h"
#include "Util.h"

#include "cm256.h"
#include "gf256.h"
#include "ubd.h"

#include "VolumeManager.cpp"

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
#endif
    return ((Volume*)context)->ReadDecrypt(buf, size, offset) ? 0 : -1;
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
#endif
    return ((Volume*)context)->WriteEncrypt(buf, size, offset) ? 0 : -1;
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
  }

  void ActionHandler::Cleanup()
  {
    printf("Unmounting volumes...\n");
    ActionHandler::Unmount("/dev/nbd",true);

    for(auto &it : volumeInfo)
    {
      printf("Cleaning %s...\n",it.second->nbdPath.c_str());
      ubd_disconnect(it.second->nbdPath.c_str());
      delete it.second;
    }
  }

  // returns {0: fail, 1: success, 2: already binded}
  int ActionHandler::BindVolume(const std::string &name, const std::string &path)
  {
    std::string nbdPath = ActionHandler::GetNextNBD();
    if(nbdPath == "") 
    {
      printf("No available block devices.\n");
      return 0;
    }

    if(volumeInfo.find(name) != volumeInfo.end())
    {
      printf("Volume is binded to '%s'.\n",volumeInfo[name]->nbdPath.c_str());
      if(!path.empty())
      {
        volumeInfo[name]->mountPath = path;
      }
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
 #ifndef __APPLE__
    volume->EnableCache(std::make_unique<dfs::Cache>("cache", volume.get(), 200, 10));
 #endif
    
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
      ActionHandler::Unmount(it->second->nbdPath);
      nbdInfo[it->second->nbdPath] = false;
      ubd_disconnect(it->second->nbdPath.c_str());
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
