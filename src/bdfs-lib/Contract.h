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
#include <vector>
#include <memory>
#include <json/json.h>
#include "Digest.h"
#include "ContractTerm.h"
#include "KeyBase.h"

namespace bdfs
{
  class Contract
  {
  public:

    Contract();

    Contract(std::string address, KeyBase * privateKey);

    bool SetRequest(std::string id, uint64_t space, size_t partitions, double price, std::string symbol);

    bool AddResponse(std::string account, uint32_t nonce);

    bool Seal(std::string transaction);

    bool FromJSON(const Json::Value & json);

    bool ToJSON(Json::Value & output) const;

    bool Verify() const;

    bool IsSealed() const;

    ContractTerm * GetLastTerm() const;

  private:

    std::unique_ptr<ContractTerm> JsonToTerm(const Json::Value & obj, const sha256_t * additional) const;

    bool GetDigest(size_t length, sha256_t result) const;

  private:

    std::vector<std::unique_ptr<ContractTerm>> terms;

    std::string address;

    KeyBase * privateKey;
  };
}