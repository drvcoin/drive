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
#include <string>
#include <fstream>
#include <streambuf>
#include <json/json.h>
#include <uuid/uuid.h>
#include <sys/stat.h>

#include "Options.h"
#include "Util.h"
#include "ContractRepository.h"

namespace bdhost
{
  uint64_t htonll(uint64_t val)
  {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (((uint64_t) htonl(val)) << 32) + htonl(val >> 32);
#else
    return val;
#endif
  }

  uint64_t ntohll(uint64_t val)
  {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (((uint64_t) ntohl(val)) << 32) + ntohl(val >> 32);
#else
    return val;
#endif
  }


  std::string uuidgen()
  {
    uuid_t obj;
    uuid_generate(obj);
    char str[36];
    uuid_unparse(obj, str);
    return str;
  }


  const char * RESERVED_FILE = "reserved.json";

  uint64_t GetReservedSpace(std::string reserve_id)
  {
    reserve_id = reserve_id == "" ? RESERVED_FILE : reserve_id + "/" + RESERVED_FILE;

    std::ifstream f(reserve_id);
    std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    Json::Reader reader;
    Json::Value json;
    if (!reader.parse(str.c_str(), str.size(), json, false) ||
        !json.isObject() ||
        !json["size"].isIntegral())
    {
      return 0;
    }

    return json["size"].asUInt();
  }

  std::string SetReservedSpace(const uint64_t size)
  {
    static uint64_t contractSize = bdcontract::ContractRepository::Load(bdhost::Options::contract.c_str())->Size();

    auto reservedSpace = GetReservedSpace();

    if(contractSize < (size + reservedSpace))
    {
      printf("Reserve size exceeds contractSize\n");
      return "";
    }

    Json::Value json;
    json["size"] = Json::Value::UInt(size + reservedSpace);

    std::ofstream out(RESERVED_FILE);
    out << json.toStyledString();
    out.close();

    std::string reserve_id = uuidgen();
    mkdir(reserve_id.c_str(), 0755);
    json["size"] = Json::Value::UInt(size + reservedSpace);

    std::ofstream id_out(reserve_id + "/" + RESERVED_FILE);
    id_out << json.toStyledString();
    id_out.close();

    return reserve_id;
  }
}
