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

#include <string.h>
#include <drive/common/Base64Encoder.h>
#include <drive/common/Buffer.h>
#include <drive/common/KeyBase.h>
#include <drive/common/ContractTerm.h>


namespace bdfs
{
  bool ContractTerm::Sign(const KeyBase & privateKey)
  {
    if (!this->GenerateDigest())
    {
      return false;
    }

    return privateKey.Sign(this->digest, sizeof(this->digest), this->signature);
  }


  bool ContractTerm::Verify(const KeyBase & publicKey)
  {
    if (!this->GenerateDigest())
    {
      return false;
    }

    return publicKey.Verify(this->digest, sizeof(this->digest), this->signature);
  }


  void ContractTerm::SetPayload(std::string val)
  {
    this->payload = std::move(val);
    this->digestGenerated = false;
  }


  void ContractTerm::SetAdditional(const sha256_t & val)
  {
    memcpy(this->additional, val, sizeof(sha256_t));
    this->digestGenerated = false;
    this->additionalSet = true;
  }


  bool ContractTerm::GetDigest(sha256_t result)
  {
    if (!this->GenerateDigest())
    {
      return false;
    }

    memcpy(result, this->digest, sizeof(sha256_t));
    return true;
  }


  bool ContractTerm::ParseContent()
  {
    Buffer data;
    if (this->payload.empty() || !data.Resize(Base64Encoder::GetDecodedLength(this->payload.size())))
    {
      return false;
    }

    size_t dataLen = Base64Encoder::Decode(
      this->payload.c_str(),
      this->payload.size(),
      static_cast<uint8_t *>(data.Buf()), data.Size());
    
    if (dataLen == 0)
    {
      return false;
    }

    Json::Reader reader;
    return reader.parse(static_cast<const char *>(data.Buf()), dataLen, this->content, false);
  }


  bool ContractTerm::GenerateDigest()
  {
    if (this->digestGenerated)
    {
      return true;
    }

    Digest sha256;

    if (!sha256.Initialize())
    {
      return false;
    }

    if (this->additionalSet && !sha256.Update(this->additional, sizeof(this->additional)))
    {
      return false;
    }

    if (!sha256.Update(this->payload.c_str(), this->payload.size()))
    {
      return false;
    }

    if (!sha256.Finish(this->digest))
    {
      return false;
    }

    this->digestGenerated = true;
    return true;
  }
}
