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

#include <map>

namespace dfs
{
  struct VolumeMeta 
  {
    std::string volumeName;
    std::string nbdPath;
    std::string mountPath;
    bool isFormatted;
    bool isMounted;
  };

  class ActionHandler
  {
    static std::map<std::string,VolumeMeta*> volumeInfo;
    static std::map<std::string, bool> nbdInfo;

    static std::string GetNextNBD();
    static void Unmount(const std::string &nbdPath, bool matchAll);

  public:
    static void Cleanup();
    static int BindVolume(const std::string &name, const std::string &path);
    static int UnbindVolume(const std::string &name);
    
    static inline void AddNbdPath(std::string path)
    {
      ActionHandler::nbdInfo[path] = false;
    }

    static inline std::string GetNbdForVolume(const std::string &name)
    {
      return volumeInfo.find(name) != volumeInfo.end() ? volumeInfo[name]->nbdPath : "";
    }

    static inline std::string GetMountPathForVolume(const std::string &name)
    {
      return volumeInfo.find(name) != volumeInfo.end() ? volumeInfo[name]->mountPath : "";
    }

    static inline const std::map<std::string,VolumeMeta *> &GetVolumeInfo()
    {
      return ActionHandler::volumeInfo;
    }
  };
}
