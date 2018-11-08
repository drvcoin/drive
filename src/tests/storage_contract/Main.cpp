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
#include <drive/common/KeyManager.h>
#include <drive/common/Contract.h>

static std::string g_clientAddress = "client";
static std::string g_hostAddressPrefix = "host-";

#define _1GB (1024ULL * 1024 * 1024)


void SaveContract(const bdfs::Contract & contract)
{
  Json::Value json;
  if (!contract.ToJSON(json))
  {
    printf("Failed to convert contract to JSON.\n");
    exit(1);
  }

  Json::FastWriter writer;
  std::string str = writer.write(json);

  printf("Contract:\n============\n%s\n============\n", str.c_str());

  FILE * output = fopen("test.contract", "w");
  if (!output)
  {
    printf("Failed to open output file.\n");
    exit(2);
  }

  fwrite(str.c_str(), 1, str.size(), output);
  fclose(output);
}


void LoadContract(bdfs::Contract & contract)
{
  FILE * input = fopen("test.contract", "r");
  if (!input)
  {
    printf("Failed to open input file.\n");
    exit(3);
  }

  fseek(input, 0L, SEEK_END);
  size_t inputSize = (size_t)ftell(input);
  rewind(input);

  std::unique_ptr<char[]> buffer(new char[inputSize]);
  fread(buffer.get(), 1, inputSize, input);
  fclose(input);

  Json::Value json;
  Json::Reader reader;
  if (!reader.parse(buffer.get(), inputSize, json, false))
  {
    printf("Failed to parse previous terms.\n");
    exit(4);
  }

  if (!contract.FromJSON(json))
  {
    printf("Failed to convert from JSON to contract.\n");
    exit(5);
  }

  printf("Verifying contract\n");
  if (!contract.Verify())
  {
    printf("Failed to verify contract.\n");
    exit(6);
  }
}


void TestRequest(size_t hostCount)
{
  printf("--- TEST REQUEST ---\n");
  printf("Creating client request.\n");
  bdfs::KeyManager keyManager;
  if (!keyManager.Initialize(g_clientAddress + ".pri", g_clientAddress + ".pub"))
  {
    printf("No key pairs found for client.\n");
    exit(101);
  }

  keyManager.PrivateKey()->Print();
  keyManager.PublicKey()->Print();

  bdfs::Contract contract{g_clientAddress, keyManager.PrivateKey()};
  if (!contract.SetRequest("test-contract-id", _1GB, hostCount, 100, "TDRV"))
  {
    printf("Failed to create client request.\n");
    exit(102);
  }

  SaveContract(contract);

  printf("Client request created.\n");
  printf("--- ---\n\n");
}


void TestResponse(size_t index)
{
  printf("--- TEST RESPONSE ---\n");
  printf("Creating test response %llu\n", index);

  char hostname[128];
  sprintf(hostname, "%s%llu", g_hostAddressPrefix.c_str(), index);

  bdfs::KeyManager keyManager;
  if (!keyManager.Initialize(std::string(hostname) + ".pri", std::string(hostname) + ".pub"))
  {
    printf("No key pairs found for host-%llu.\n", index);
    exit(201);
  }

  keyManager.PrivateKey()->Print();
  keyManager.PublicKey()->Print();

  bdfs::Contract contract{hostname, keyManager.PrivateKey()};

  LoadContract(contract);

  if (!contract.AddResponse(hostname, index))
  {
    printf("Failed to add response.\n");
    exit(202);
  }

  SaveContract(contract);

  printf("Host %llu response created.\n", index);
  printf("--- ---\n\n");
}


void TestSeal()
{
  printf("--- TEST SEAL ---\n");

  printf("Sealing test contract.\n");

  bdfs::KeyManager keyManager;
  if (!keyManager.Initialize(g_clientAddress + ".pri", g_clientAddress + ".pub"))
  {
    printf("No key pairs found for client.\n");
    exit(301);
  }

  bdfs::Contract contract{g_clientAddress, keyManager.PrivateKey()};
  LoadContract(contract);

  if (!contract.Seal("test-transaction-id"))
  {
    printf("Failed to seal test contract.\n");
    exit(302);
  }

  SaveContract(contract);

  printf("Contract sealed.\n");
  printf("--- ---\n\n");
}


void VerifyContract()
{
  printf("--- VERIFY CONTRACT ---\n");

  printf("Verifying contract.\n");

  bdfs::Contract contract;
  LoadContract(contract);

  if (!contract.IsSealed())
  {
    printf("Contract seal state is wrong.\n");
    exit(401);
  }

  printf("Contract verfied.\n");
  printf("--- ---\n\n");
}


int main(int argc, const char ** argv)
{
  if (argc < 2)
  {
    printf("Usage: test-storage-contract <num_of_hosts>\n");
    printf("Note: Make sure key pair exists as \"client|host-{id}\".\"pri|pub\" in the current folder.\n");
    return -1;
  }

  size_t hostCount = static_cast<size_t>(atoi(argv[1]));

  TestRequest(hostCount);
  for (size_t i = 0; i < hostCount; ++i)
  {
    TestResponse(i);
  }
  TestSeal();

  VerifyContract();
}
