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

#include <stdlib.h>
#include <drive/common/NullKey.h>
#include <drive/common/KeyManager.h>


namespace bdfs
{
  static Buffer createTestBuffer()
  {
    Buffer result;
    result.Resize(16);
    
    uint8_t * buf = static_cast<uint8_t *>(result.Buf());
    for (size_t i = 0; i < result.Size(); ++i)
    {
      buf[i] = static_cast<uint8_t>(rand() % 256);
    }

    return std::move(result);
  }


  bool KeyManager::Initialize(std::string priKeyPath, std::string pubKeyPath)
  {
    // TODO: replace NullKey with actual key implementation
    std::unique_ptr<KeyBase> pri(new NullKey(true));
    std::unique_ptr<KeyBase> pub(new NullKey(false));

    if (!pri->Load(priKeyPath) || !pub->Load(pubKeyPath))
    {
      // TODO: replace this with actual key generation
      pri->SetData(createTestBuffer());
      pub->SetData(createTestBuffer());

      if (!pri->Save(priKeyPath) || !pub->Save(pubKeyPath))
      {
        return false;
      }
    }

    this->priKey = std::move(pri);
    this->pubKey = std::move(pub);

    return true;
  }


  std::unique_ptr<KeyBase> KeyManager::Recover(const void * data, size_t len, std::string signature, std::string sender)
  {
    // TODO: replace NullKey with actual key implementation
    std::unique_ptr<KeyBase> key(new NullKey(false));

    if (!key->Recover(data, len, signature, sender))
    {
      return nullptr;
    }

    return std::move(key);
  }
}
