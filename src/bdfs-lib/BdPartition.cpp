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

#include <json/json.h>
#include <drive/common/BdTypes.h>
#include <drive/common/Base64Encoder.h>
#include <drive/common/BdPartition.h>

namespace bdfs
{
  BD_TYPE_REG(Partition, BdPartition);


  BdPartition::BdPartition(const char * base, const char * name, const char * path, const char * type)
    : BdObject(base, name, path, type)
  {
  }


  AsyncResultPtr<ssize_t> BdPartition::Write(uint64_t blockId, uint32_t offset, const void * data, uint32_t size)
  {
    BdObject::CArgs args;
    args["block"] = Json::Value::UInt(blockId);
    args["offset"] = Json::Value::UInt(offset);

    auto result = std::make_shared<AsyncResult<ssize_t>>();

    bool rtn = this->Call("WriteBlock", args, data, size,
      [result](Json::Value & response, bool error)
      {
        if (error || !response.isInt())
        {
          result->Complete(-1);
        }
        else
        {
          result->Complete(static_cast<ssize_t>(response.asInt()));
        }
      }
    );

    return rtn ? result : nullptr;
  }


  AsyncResultPtr<std::string> BdPartition::Read(uint64_t blockId, uint32_t offset, uint32_t size)
  {
    BdObject::CArgs args;
    args["block"] = Json::Value::UInt(blockId);
    args["offset"] = Json::Value::UInt(offset);
    args["size"] = Json::Value::UInt(size);

    auto result = std::make_shared<AsyncResult<std::string>>();

    bool rtn = this->Call("ReadBlock", args,
      [result, size](std::string && data, bool error)
      {
        if (error)
        {
          result->Complete(std::string());
        }
        else
        {
          result->Complete(std::move(data));
        }
      }
    );

    return rtn ? result : nullptr;
  }


  AsyncResultPtr<bool> BdPartition::Delete()
  {
    BdObject::CArgs args;

    auto result = std::make_shared<AsyncResult<bool>>();

    bool rtn = this->Call("Delete", args,
      [result](Json::Value & response, bool error)
      {
        if (error || !response.isBool())
        {
          result->Complete(false);
        }
        else
        {
          result->Complete(response.asBool());
        }
      }
    );

    return rtn ? result : nullptr;
  }
}
