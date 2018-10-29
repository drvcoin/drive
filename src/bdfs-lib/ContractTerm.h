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

#include <stdint.h>
#include <string>
#include <json/json.h>
#include "Digest.h"

namespace bdfs
{
  class KeyBase;

  class ContractTerm
  {
  public:

    const std::string & Payload() const   { return this->payload; }
    const std::string & Signature() const { return this->signature; }
    const Json::Value & Content() const   { return this->content; }

    void SetSignature(std::string val)    { this->signature = std::move(val); }

    void SetPayload(std::string val);
    void SetAdditional(const sha256_t & val);

    bool GetDigest(sha256_t result);

    bool Sign(const KeyBase & privateKey);
    bool Verify(const KeyBase & publicKey);

    bool ParseContent();

  private:

    bool GenerateDigest();

  private:

    std::string payload;

    std::string signature;

    Json::Value content;

    sha256_t additional;

    sha256_t digest;

    bool digestGenerated = false;

    bool additionalSet = false;
  };
}
