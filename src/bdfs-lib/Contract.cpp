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

#include <assert.h>
#include <string.h>
#include <drive/common/Base64Encoder.h>
#include <drive/common/Buffer.h>
#include <drive/common/KeyManager.h>
#include <drive/common/Contract.h>


namespace bdfs
{
  Contract::Contract()
    : privateKey(nullptr)
  {
  }


  Contract::Contract(std::string address, KeyBase * privateKey)
    : address(address)
    , privateKey(privateKey)
  {
  }


  bool Contract::SetRequest(std::string id, uint64_t space, size_t partitions, double price, std::string symbol)
  {
    if (!this->privateKey || !this->terms.empty())
    {
      return false;
    }

    Json::Value obj;
    obj["Id"] = id;
    obj["Space"] = Json::UInt(space);
    obj["Partitions"] = Json::UInt(partitions);
    obj["Price"] = price;
    obj["Symbol"] = symbol;
    obj["Sender"] = this->address;

    auto term = this->JsonToTerm(obj, nullptr);
    if (!term)
    {
      return false;
    }

    this->terms.emplace_back(std::move(term));
    return true;
  }


  bool Contract::AddResponse(std::string account, uint32_t nonce)
  {
    if (!this->privateKey || this->terms.empty() || this->IsSealed())
    {
      return false;
    }

    sha256_t digest;
    if (!this->GetDigest(this->terms.size(), digest))
    {
      return false;
    }

    Json::Value obj;
    obj["Sender"] = this->address;
    obj["Account"] = account;
    obj["Nonce"] = Json::UInt(nonce);

    auto term = this->JsonToTerm(obj, & digest);
    if (!term)
    {
      return false;
    }

    this->terms.emplace_back(std::move(term));
    return true;
  }


  bool Contract::Seal(std::string transaction)
  {
    if (!this->privateKey || this->terms.empty() || this->IsSealed())
    {
      return false;
    }

    const Json::Value & firstSender = this->terms[0]->Content()["Sender"];
    if (!firstSender.isString() || firstSender.asString() != this->address)
    {
      return false;
    }

    sha256_t digest;
    if (!this->GetDigest(this->terms.size(), digest))
    {
      return false;
    }

    Json::Value obj;
    obj["Sender"] = this->address;
    obj["Transaction"] = transaction;

    auto term = this->JsonToTerm(obj, & digest);
    if (!term)
    {
      return false;
    }

    this->terms.emplace_back(std::move(term));
    return true;
  }


  bool Contract::FromJSON(const Json::Value & json)
  {
    this->terms.clear();

    if (!json.isArray() || json.isNull())
    {
      return false;
    }

    if (json.size() == 0)
    {
      return true;
    }

    for (size_t i = 0; i < json.size(); ++i)
    {
      if (!json[i].isObject() || json[i].isNull() || !json[i]["p"].isString() || !json[i]["s"].isString())
      {
        return false;
      }

      auto term = std::make_unique<ContractTerm>();
      term->SetPayload(json[i]["p"].asString());
      term->SetSignature(json[i]["s"].asString());
      if (!term->ParseContent())
      {
        return false;
      }

      this->terms.emplace_back(std::move(term));
    }

    return true;
  }


  bool Contract::ToJSON(Json::Value & output) const
  {
    output = Json::Value(Json::ValueType::arrayValue);

    for (const auto & term : this->terms)
    {
      Json::Value obj;
      obj["p"] = term->Payload();
      obj["s"] = term->Signature();
      output.append(obj);
    }

    return true;
  }


  bool Contract::Verify() const
  {
    sha256_t digest;
    bool first = true;

    for (const auto & term : this->terms)
    {
      if (!first)
      {
        term->SetAdditional(digest);
      }

      first = false;

      if (!term->GetDigest(digest))
      {
        return false;
      }

      const Json::Value & sender = term->Content()["Sender"];
      if (!sender.isString())
      {
        return false;
      }

      auto key = KeyManager::Recover(digest, sizeof(digest), term->Signature(), sender.asString());
      if (!term->Verify(*key))
      {
        return false;
      }
    }

    return true;
  }


  bool Contract::IsSealed() const
  {
    if (this->terms.size() < 2)
    {
      return false;
    }

    const Json::Value & firstSender = this->terms[0]->Content()["Sender"];
    const Json::Value & lastSender = this->terms[this->terms.size() - 1]->Content()["Sender"];

    return firstSender.isString() && lastSender.isString() && firstSender.asString() == lastSender.asString();
  }


  ContractTerm * Contract::GetLastTerm() const
  {
    if (this->terms.empty())
    {
      return nullptr;
    }

    return this->terms[this->terms.size() - 1].get();
  }


  std::unique_ptr<ContractTerm> Contract::JsonToTerm(const Json::Value & obj, const sha256_t * additional) const
  {
    assert(this->privateKey);

    Json::FastWriter writer;
    std::string json = writer.write(obj);

    Buffer buffer;
    if (!buffer.Resize(Base64Encoder::GetEncodedLength(json.size())))
    {
      return nullptr;
    }

    size_t size = Base64Encoder::Encode(
      reinterpret_cast<const uint8_t *>(json.c_str()),
      json.size(),
      static_cast<char *>(buffer.Buf()),
      buffer.Size());

    auto term = std::make_unique<ContractTerm>();
    term->SetPayload(std::string(static_cast<const char *>(buffer.Buf()), buffer.Size()));

    if (additional)
    {
      term->SetAdditional(* additional);
    }

    if (!term->Sign(* this->privateKey))
    {
      return nullptr;
    }

    return std::move(term);
  }


  bool Contract::GetDigest(size_t length, sha256_t result) const
  {
    if (this->terms.empty())
    {
      return false;
    }

    sha256_t digest;
    for (size_t i = 0; i < length; ++i)
    {
      if (i > 0)
      {
        this->terms[i]->SetAdditional(digest);
      }

      if (!this->terms[i]->GetDigest(digest))
      {
        return false;
      }
    }

    memcpy(result, digest, sizeof(sha256_t));
    return true;
  }
}
