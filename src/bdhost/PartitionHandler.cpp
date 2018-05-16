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

#include <string>
#include "HttpHandlerRegister.h"
#include "Partition.h"
#include "Base64Encoder.h"
#include "PartitionHandler.h"

namespace bdhost
{
  static const char PATH[] = "/api/host/Partitions";

  REGISTER_HTTP_HANDLER(Partition, PATH, new PartitionHandler());

  void PartitionHandler::ProcessRequest(HttpContext & context)
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
        context.writeError("Failed", "Object not found", ErrorCode::OBJECT_NOT_FOUND);
      }

      return;
    }

    auto pos = path.find('/', sizeof(PATH));
    if (pos == std::string::npos)
    {
      context.setResponseCode(404);
      context.writeError("Failed", "Object not found", ErrorCode::OBJECT_NOT_FOUND);
      return;
    }

    auto name = path.substr(sizeof(PATH), pos - sizeof(PATH));
    auto action = pos < path.size() - 1 ? path.substr(pos + 1) : std::string();

    this->OnRequest(context, name, action);
  }


  void PartitionHandler::OnRequest(HttpContext & context, const std::string & name, const std::string & action)
  {
    bool found = false;
    struct stat st = {0};
    uint64_t blockCount = 0;
    uint64_t blockSize = 0;
    if (!name.empty() && name != "." && name != ".." && stat(name.c_str(), &st) == 0)
    {
      if (S_ISDIR(st.st_mode))
      {
        FILE * config = fopen((name + "/.config").c_str(), "r");
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

    if (!found)
    {
      context.setResponseCode(404);
      context.writeError("Failed", "Object not found", ErrorCode::OBJECT_NOT_FOUND);
      return;
    }

    if (action.empty())
    {
      std::stringstream res;
      res << "{\"Name\":\"" << name << "\",\"Path\":\"host://Partitions/" << name << "\",\"Type\":\"Partition\"";
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
  }


  void PartitionHandler::OnReadBlock(HttpContext & context, const std::string & name, uint64_t blockCount, uint64_t blockSize)
  {
    uint64_t blockId = static_cast<uint64_t>(strtoull(context.parameter("block"), nullptr, 10));
    uint32_t offset = static_cast<uint32_t>(strtoul(context.parameter("offset"), nullptr, 10));
    uint32_t size = static_cast<uint32_t>(strtoul(context.parameter("size"), nullptr, 10));

    if (blockId >= blockCount || offset >= blockSize || size > blockSize - offset)
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Invalid arguments", ErrorCode::ARGUMENT_INVALID);
      return;
    }

    uint8_t * buffer = new uint8_t[size];
    memset(buffer, 0, size);

    Partition partition{name.c_str(), blockCount, blockSize};
    
    if (partition.ReadBlock(blockId, buffer, size, offset))
    {
      std::string encoded;
      size_t encodedSize = Base64Encoder::GetEncodedLength(size);
      encoded.resize(encodedSize + 2);
      encoded[0] = '"';

      encodedSize = Base64Encoder::Encode(buffer, size, &encoded[1], encodedSize);
      encoded[encodedSize + 1] = '"';

      context.writeResponse(encoded);
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Failed to read block", ErrorCode::GENERIC_ERROR);
    }

    delete[] buffer;
  }


  void PartitionHandler::OnWriteBlock(HttpContext & context, const std::string & name, uint64_t blockCount, uint64_t blockSize)
  {
    uint64_t blockId = static_cast<uint64_t>(strtoull(context.parameter("block"), nullptr, 10));
    uint32_t offset = static_cast<uint32_t>(strtoul(context.parameter("offset"), nullptr, 10));
    std::string encoded = context.parameter("data");

    uint32_t size = static_cast<uint32_t>(Base64Encoder::GetDecodedLength(encoded.size()));

    if (blockId >= blockCount || offset >= blockSize)
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Invalid arguments", ErrorCode::ARGUMENT_INVALID);
      return;
    }

    uint8_t * buffer = new uint8_t[size];
    size = Base64Encoder::Decode(encoded.c_str(), encoded.size(), buffer, size);

    Partition partition{name.c_str(), blockCount, blockSize};
    if (partition.WriteBlock(blockId, buffer, size, offset))
    {
      char sizeStr[64];
      sprintf(sizeStr, "%d", size);
      context.writeResponse(sizeStr);
    }
    else
    {
      context.setResponseCode(500);
      context.writeError("Failed", "Failed to write block", ErrorCode::GENERIC_ERROR);
    }

    delete[] buffer;

  }
}
