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


#include <stdio.h>
#include <memory>
#include <string.h>
#include <drive/common/HexEncoder.h>
#include <drive/common/ScopeGuard.h>
#include <drive/common/KeyBase.h>

namespace bdfs
{
  KeyBase::KeyBase(bool isPrivate)
    : isPrivate(isPrivate)
  {
  }


  void KeyBase::SetData(Buffer data)
  {
    this->data = std::move(data);
  }


  void KeyBase::SetData(const uint8_t * data, size_t len)
  {
    this->data.Resize(len);
    memcpy(this->data.Buf(), data, len);
  }


  bool KeyBase::Save(std::string filename) const
  {
    size_t hexLen = HexEncoder::GetEncodedLength(this->data.Size());
    std::unique_ptr<char[]> hex(new char[hexLen]);
    if (!HexEncoder::Encode(static_cast<const uint8_t *>(this->data.Buf()), this->data.Size(), hex.get(), hexLen))
    {
      return false;
    }

    bool success = false;
    FILE * file = fopen(filename.c_str(), "w");
    if (file)
    {
      success = (fwrite(hex.get(), 1, hexLen, file) == hexLen);
      fclose(file);
    }

    return success;
  }


  bool KeyBase::Load(std::string filename)
  {
    FILE * file = fopen(filename.c_str(), "r");
    if (!file)
    {
      return false;
    }

    ScopeGuard guard([file] {
      if (file)
      {
        fclose(file);
      }
    });

    fseek(file, 0, SEEK_END);
    auto length = ftell(file);
    if (length <= 0)
    {
      return false;
    }

    rewind(file);
    std::unique_ptr<char[]> hex(new char[length]);

    if (fread(hex.get(), 1, length, file) != length)
    {
      return false;
    }

    if (!this->data.Resize(HexEncoder::GetDecodedLength(length)))
    {
      return false;
    }

    return HexEncoder::Decode(hex.get(),
      static_cast<size_t>(length),
      static_cast<uint8_t *>(this->data.Buf()),
      this->data.Size()) == this->data.Size();
  }


  bool KeyBase::Sign(const void * data, size_t len, std::string & output) const
  {
    return false;
  }


  bool KeyBase::Verify(const void * data, size_t len, std::string signature) const
  {
    return false;
  }


  bool KeyBase::Recover(const void * data, size_t len, std::string signature, std::string sender)
  {
    return false;
  }

  void KeyBase::Print()
  {
    size_t hexLen = HexEncoder::GetEncodedLength(this->data.Size());
    std::unique_ptr<char[]> hex(new char[hexLen]);
    if (!HexEncoder::Encode(static_cast<const uint8_t *>(this->data.Buf()), this->data.Size(), hex.get(), hexLen))
    {
      printf("Error\n");
    }

    if (this->IsPrivate())
    {
      printf("[PRIVATE KEY] %s\n", hex.get());
    }
    else
    {
      printf("[PUBLIC KEY] %s\n", hex.get());
    }
  }
}
