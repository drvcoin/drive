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
#include <map>
#include <vector>
#include <limits>
#include "Folder.h"
#include "BlobError.h"
#include "BlobProvider.h"

namespace bdblob
{
  class BlobApi
  {
  public:

    bool Initialize(std::unique_ptr<BlobProvider> provider, std::string rootId);

    std::map<std::string, Folder::Entry> List(std::string path);

    bool Mkdir(std::string path);

    bool Put(std::string path, std::string localPath, bool force = false);

    bool Get(std::string path, std::string localPath, bool force = false);

    bool Get(std::string path, uint64_t offset, void * output, uint64_t * size);

    bool Delete(std::string path);

    std::unique_ptr<Folder::Entry> Info(std::string path);

    std::shared_ptr<Folder> GetParentFolder(const std::vector<std::string> & parts);

    BlobError Error() const     { return this->error; }

    std::string RootId() const
    {
      return this->root ? this->root->Properties().id : std::string();
    }

  public:

    static std::vector<std::string> SplitPath(std::string path);

    static std::string JoinPaths(const std::vector<std::string> & parts,
                                 size_t offset = 0,
                                 size_t len = std::numeric_limits<size_t>::max());

    static bool PatternMatch(std::string pattern, std::string target);
    
    static std::string BaseName(std::string path);

  private:

    void ClearError()     { this->error = BlobError::SUCCESS; }

    bool ParsePutTarget(const std::string & path,
                        const std::string & localPath,
                        std::shared_ptr<Folder> & folder,
                        std::string & filename);
    
  private:

    std::unique_ptr<BlobProvider> provider;

    std::shared_ptr<Folder> root;

    BlobError error = BlobError::SUCCESS;
  };
}