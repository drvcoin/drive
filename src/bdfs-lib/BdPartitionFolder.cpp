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
#include "BdTypes.h"
#include "BdSession.h"
#include "BdPartitionFolder.h"

namespace bdfs
{
  BD_TYPE_REG(PartitionFolder, BdPartitionFolder);


  BdPartitionFolder::BdPartitionFolder(const char * base, const char * name, const char * path, const char * type)
    : BdObject(base, name, path, type)
  {
  }


  AsyncResultPtr<std::shared_ptr<BdPartition>> BdPartitionFolder::CreatePartition(const std::string & reserveId, uint32_t blockSize)
  {
    BdObject::CArgs args;
    args["reserveId"] = reserveId;
    args["block"] = Json::Value::UInt(blockSize);

    auto result = std::make_shared<AsyncResult<std::shared_ptr<BdPartition>>>();

    bool rtn = this->Call("NewPartition", args,
      [this, result](Json::Value & response, bool error)
      {
        if (error || !response.isObject() ||
            !response["Type"].isString() || response["Type"].asString() != "Partition" ||
            !response["Path"].isString() || !response["Name"].isString())
        {
          result->Complete(nullptr);
        }
        else
        {
          auto session = BdSession::GetSession(this->Base());
          auto obj = session->CreateObject(response["Name"].asCString(), response["Path"].asCString(), "Partition");
          result->Complete(std::static_pointer_cast<BdPartition>(obj));
        }
      }
    );

    return rtn ? result : nullptr;
  }


    AsyncResultPtr<std::string> BdPartitionFolder::ReservePartition(const uint64_t size)
    {
      BdObject::CArgs args;
      args["size"] = size;;

      auto result = std::make_shared<AsyncResult<std::string>>();

      bool rtn = this->Call("Reserve", args,
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
}
