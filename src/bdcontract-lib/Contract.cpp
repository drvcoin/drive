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
#include "Contract.h"

namespace bdcontract
{
  std::string Contract::ToString() const
  {
    Json::Value json;
    json["name"] = this->name;
    json["consumer"] = this->consumer;
    json["provider"] = this->provider;
    json["size"] = Json::Value::UInt(this->size);

    return json.toStyledString();
  }


  std::unique_ptr<Contract> Contract::FromString(const char * str, size_t len)
  {
    Json::Reader reader;
    Json::Value json;
    if (!reader.parse(str, len, json, false) ||
        !json.isObject() ||
        !json["name"].isString() ||
        !json["consumer"].isString() ||
        !json["provider"].isString() ||
        !json["size"].isIntegral())
    {
      return nullptr;
    }

    auto contract = std::make_unique<Contract>();
    contract->SetName(json["name"].asString());
    contract->SetConsumer(json["consumer"].asString());
    contract->SetProvider(json["provider"].asString());
    contract->SetSize(json["size"].asUInt());

    return std::move(contract);
  }


  std::unique_ptr<Contract> Contract::FromString(const std::string & str)
  {
    return FromString(str.c_str(), str.size());
  }
}
