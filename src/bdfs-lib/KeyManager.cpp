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
#include "EcdsaKey.h"
#include "KeyManager.h"

#include <openssl/ec.h>
#include <openssl/ssl.h>


namespace bdfs
{
  bool KeyManager::GenerateKeys(uint8_t * private_key, uint8_t * public_key)
  {
    EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);

    if (!EC_KEY_generate_key(ec_key))
    {
      return false;
    }

    const BIGNUM * bn_private_key;
    bn_private_key = EC_KEY_get0_private_key(ec_key);

    BN_bn2bin(bn_private_key,private_key);


    const EC_POINT * ecpoint_public_key;
    ecpoint_public_key = EC_KEY_get0_public_key(ec_key);

    BIGNUM * bn_public_key = BN_new();
    if(!EC_POINT_point2bn(EC_KEY_get0_group(ec_key), EC_KEY_get0_public_key(ec_key), POINT_CONVERSION_UNCOMPRESSED, bn_public_key, NULL))
    {
      return false;
    }

    BN_bn2bin(bn_public_key,public_key);

    if (bn_public_key)
    {
      BN_clear_free(bn_public_key);
    }

    if (ec_key)
    {
      EC_KEY_free(ec_key);
    }

    return true;
  }

  bool KeyManager::Initialize(std::string priKeyPath, std::string pubKeyPath)
  {
    std::unique_ptr<KeyBase> pri(new EcdsaKey(true));
    std::unique_ptr<KeyBase> pub(new EcdsaKey(false));

    if (!pri->Load(priKeyPath) || !pub->Load(pubKeyPath))
    {
      uint8_t private_key[32];
      uint8_t public_key[65];

      if (!GenerateKeys(private_key, public_key))
      {
        return false;
      }

      pri->SetData(private_key, 32);
      pub->SetData(public_key, 65);

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
    std::unique_ptr<KeyBase> key(new EcdsaKey(false));

    if (!key->Recover(data, len, signature, sender))
    {
      return nullptr;
    }

    return std::move(key);
  }
}
