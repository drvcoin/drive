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

#include <drive/common/EcdsaKey.h>


#define USE_NUM_NONE
#define USE_FIELD_10X26
#define USE_FIELD_INV_BUILTIN
#define USE_SCALAR_8X32
#define USE_SCALAR_INV_BUILTIN
#include "include/secp256k1.h"
#include "src/secp256k1.c"
#include "src/modules/recovery/main_impl.h"


namespace bdfs
{
  EcdsaKey::EcdsaKey(bool isPrivate)
    : KeyBase(isPrivate)
  {
  }


  bool EcdsaKey::Sign(const void * data, size_t len, std::string & output) const
  {
    if (!this->IsPrivate())
    {
      return false;
    }

    secp256k1_context * ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);

    int rc;
    const unsigned char* private_key = (const unsigned char*) this->Data();
    unsigned char * digest = (unsigned char *) data;


    secp256k1_ecdsa_recoverable_signature rsignature;
    rc = secp256k1_ecdsa_sign_recoverable(ctx, &rsignature, digest, private_key, NULL, NULL);
    if (rc == 0)
    {
      return false;
    }

    int recid = 0;
    int sgnLen = 64;
    int hexLen = HexEncoder::GetEncodedLength(sgnLen);
    uint8_t compact[sgnLen];
    secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &rsignature);


    char compactHex[hexLen];
    if (HexEncoder::Encode(compact,sgnLen,compactHex,hexLen) == hexLen)
    {
      output.assign(compactHex,hexLen);
      return true;
    }
    else
    {
      return false;
    }
  }


  bool EcdsaKey::Verify(const void * data, size_t len, std::string signature) const
  {
    if (this->IsPrivate())
    {
      return false;
    }

    secp256k1_context * ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

    size_t hexLen = signature.size();
    size_t sgnLen = HexEncoder::GetDecodedLength(hexLen);
    uint8_t compact[sgnLen];
    if (HexEncoder::Decode(signature.c_str(),hexLen,compact,sgnLen) != sgnLen)
    {
      return false;
    }


    int rc;
    secp256k1_ecdsa_signature normsignature;
    secp256k1_ecdsa_recoverable_signature rsignature;

    rc = secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &rsignature, compact,0);
    if (rc == 0)
    {
      return false;
    }


    secp256k1_ecdsa_recoverable_signature_convert(ctx,&normsignature,&rsignature);

    const unsigned char * public_key = (const unsigned char*) this->Data();
    unsigned char * digest = (unsigned char *) data;

    secp256k1_pubkey pubkey;
    rc = secp256k1_ec_pubkey_parse(ctx, &pubkey, public_key, 65);
    if (rc == 0)
    {
      return false;
    }


    rc = secp256k1_ecdsa_verify(ctx, &normsignature, digest, &pubkey);
    if (rc == 0)
    {
      return false;
    }

    return true;
  }


  bool EcdsaKey::Recover(const void * data, size_t len, std::string signature, std::string sender)
  {
    secp256k1_context * ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

    size_t hexLen = signature.size();
    size_t sgnLen = HexEncoder::GetDecodedLength(hexLen);
    uint8_t compact[sgnLen];
    if (HexEncoder::Decode(signature.c_str(),hexLen,compact,sgnLen) != sgnLen)
    {
      return false;
    }


    int rc;
    secp256k1_ecdsa_recoverable_signature rsignature;

    rc = secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &rsignature, compact,0);
    if (rc == 0)
    {
      return false;
    }

    unsigned char * digest = (unsigned char *) data;
    secp256k1_pubkey recovered_public_key;
    rc = secp256k1_ecdsa_recover(ctx, &recovered_public_key, &rsignature, digest);
    if (rc == 0)
    {
      return false;
    }


    size_t keyLen = 65;
    uint8_t spubkey[keyLen];
    secp256k1_ec_pubkey_serialize(ctx, spubkey, &keyLen, &recovered_public_key, SECP256K1_EC_UNCOMPRESSED);

    this->SetData(spubkey, keyLen);

    return true;
  }
}
