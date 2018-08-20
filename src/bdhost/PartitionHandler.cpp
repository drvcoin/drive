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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <assert.h>
#include "Util.h"
#include "Options.h"
#include "HttpHandlerRegister.h"
#include "Partition.h"
#include "PartitionHandler.h"

namespace bdhost
{
  static const char PATH[] = "/api/host/Partitions";

  REGISTER_HTTP_HANDLER(Partition, PATH, new PartitionHandler());

  void PartitionHandler::ProcessRequest(bdhttp::HttpContext & context)
  {
    std::string path = context.path();
    if (path.size() <= sizeof(PATH) || path[sizeof(PATH) - 1] != '/')
    {
      if (path.size() == sizeof(PATH) && path == std::string(PATH) + '/')
      {
        context.writeResponse("{\"Name\":\"Partitions\",\"Path\":\"host://Partitions\",\"Type\":\"PartitionFolder\"}");
      }
      else
      {
        context.setResponseCode(404);
        context.writeError("Failed", "Object not found", bdhttp::ErrorCode::OBJECT_NOT_FOUND);
      }

      return;
    }

    auto pos = path.find('/', sizeof(PATH));
    if (pos == std::string::npos)
    {
      auto action = path.substr(sizeof(PATH));

      this->OnFolderRequest(context, action);
    }
    else
    {
      auto name = path.substr(sizeof(PATH), pos - sizeof(PATH));
      auto action = pos < path.size() - 1 ? path.substr(pos + 1) : std::string();

      this->OnPartitionRequest(context, name, action);
    }
  }


  void PartitionHandler::OnFolderRequest(bdhttp::HttpContext & context, const std::string & action)
  {
    if (action == "NewPartition")
    {
      this->OnCreatePartition(context);
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Action not supported", bdhttp::ErrorCode::NOT_SUPPORTED);
    }
  }


  void PartitionHandler::OnCreatePartition(bdhttp::HttpContext & context)
  {
    // TODO: add reference to contract and prevent creation on executed contract
    // TODO: validate consumer and provider identity from contract

    // TODO: client is using contract id for the request which we do not use anymore
    //       change the client to use reservation id

    uint64_t blockSize = static_cast<uint64_t>(strtoull(context.parameter("block"), nullptr, 10));
    if (blockSize == 0)
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Block size should not be 0", bdhttp::ErrorCode::ARGUMENT_INVALID);
      return;
    }

    uint64_t blockCount = Options::size / blockSize;

    if (blockCount == 0)
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Block size is too large", bdhttp::ErrorCode::ARGUMENT_INVALID);
      return;
    }

    std::string uuid = uuidgen();
    std::string uuidPath = std::string(WORK_DIR) + uuid;

    mkdir(uuidPath.c_str(), 0755);

    FILE * config = fopen((uuidPath + "/.config").c_str(), "w");
    if (!config)
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Failed to initialize partition", bdhttp::ErrorCode::GENERIC_ERROR);
      return;
    }

    fwrite(&blockCount, 1, sizeof(blockCount), config);
    fwrite(&blockSize, 1, sizeof(blockSize), config);

    fclose(config);

    std::stringstream res;
    res << "{"
        << "\"Name\":\"" << uuid << "\","
        << "\"Path\":\"host://Partitions/" << uuid << "\","
        << "\"Type\":\"Partition\""
        << "}";

    context.writeResponse(res.str());
  }


  void PartitionHandler::OnPartitionRequest(bdhttp::HttpContext & context, const std::string & name, const std::string & action)
  {
    bool found = false;
    struct stat st = {0};
    uint64_t blockCount = 0;
    uint64_t blockSize = 0;

    if (!name.empty() && name != "." && name != "..")
    {
      std::string namePath = std::string(WORK_DIR) + name;
      if (stat(namePath.c_str(), &st) == 0)
      {
        if (S_ISDIR(st.st_mode))
        {
          FILE * config = fopen((namePath + "/.config").c_str(), "r");
          if (config)
          {
            if (fread(&blockCount, 1, sizeof(uint64_t), config) == sizeof(uint64_t) &&
                fread(&blockSize, 1, sizeof(uint64_t), config) == sizeof(uint64_t))
            {
              found = true;
            }

            fclose(config);
          }
        }
      }
    }

    if (!found)
    {
      context.setResponseCode(404);
      context.writeError("Failed", "Object not found", bdhttp::ErrorCode::OBJECT_NOT_FOUND);
      return;
    }

    if (action.empty())
    {
      std::stringstream res;
      res << "{\"Name\":\"" << name << "\",\"Path\":\"host://Partitions/" << name << "\",\"Type\":\"Partition\"}";
      context.writeResponse(res.str());
    }
    else if (action == "ReadBlock")
    {
      this->OnReadBlock(context, name, blockCount, blockSize);
    }
    else if (action == "WriteBlock")
    {
      this->OnWriteBlock(context, name, blockCount, blockSize);
    }
    else if (action == "Delete")
    {
      this->OnDelete(context, name);
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Action not supported", bdhttp::ErrorCode::NOT_SUPPORTED);
    }
  }


  void PartitionHandler::OnReadBlock(bdhttp::HttpContext & context, const std::string & name, uint64_t blockCount, uint64_t blockSize)
  {
    uint64_t blockId = static_cast<uint64_t>(strtoull(context.parameter("block"), nullptr, 10));
    uint32_t offset = static_cast<uint32_t>(strtoul(context.parameter("offset"), nullptr, 10));
    uint32_t size = static_cast<uint32_t>(strtoul(context.parameter("size"), nullptr, 10));

    if (blockId >= blockCount || offset >= blockSize || size > blockSize - offset)
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Invalid arguments", bdhttp::ErrorCode::ARGUMENT_INVALID);
      return;
    }

    uint8_t * buffer = new uint8_t[size];
    memset(buffer, 0, size);

    Partition partition{name.c_str(), blockCount, blockSize};

    if (partition.ReadBlock(blockId, buffer, size, offset))
    {
      context.addResponseHeader("Content-Type", "application/octet-stream");
      context.writeResponse(buffer, size);
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Failed to read block", bdhttp::ErrorCode::GENERIC_ERROR);
    }

    delete[] buffer;
  }


  void PartitionHandler::OnWriteBlock(bdhttp::HttpContext & context, const std::string & name, uint64_t blockCount, uint64_t blockSize)
  {
    uint64_t blockId = static_cast<uint64_t>(strtoull(context.parameter("block"), nullptr, 10));
    uint32_t offset = static_cast<uint32_t>(strtoul(context.parameter("offset"), nullptr, 10));
    size_t size = context.bodylen();
    const void * data = context.body();

    if (blockId >= blockCount || offset >= blockSize || !data || size == 0)
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Invalid arguments", bdhttp::ErrorCode::ARGUMENT_INVALID);
      return;
    }

    Partition partition{name.c_str(), blockCount, blockSize};
    if (partition.WriteBlock(blockId, data, size, offset))
    {
      char sizeStr[64];
      sprintf(sizeStr, "%llu", (unsigned long long)size);
      context.writeResponse(sizeStr);
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Failed to write block", bdhttp::ErrorCode::GENERIC_ERROR);
    }
  }


  void PartitionHandler::OnDelete(bdhttp::HttpContext & context, const std::string & name)
  {
    // TODO: release the reference to contract

    std::stringstream cmd;
    cmd << "rm -rf " << name;

    system(cmd.str().c_str());

    context.writeResponse("true");
  }
}

