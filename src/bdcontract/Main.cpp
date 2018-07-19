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

#include <inttypes.h>
#include <stdio.h>
#include "Options.h"
#include "ContractRepository.h"


int Create(bdcontract::ContractRepository & repo,
    const std::string & name,
    const std::string & provider,
    uint64_t size,
    uint32_t reputation)
{
  if (repo.LoadContract(name.c_str()))
  {
    printf("Error: contract already exists.\n");
    return -2;
  }

  bdcontract::Contract contract;
  contract.SetName(name);
  contract.SetProvider(provider);
  contract.SetSize(size);
  contract.SetReputation(reputation);

  if (!repo.SaveContract(contract))
  {
    printf("Error: failed to save contract.\n");
    return -3;
  }

  return 0;
}


int List(bdcontract::ContractRepository & repo)
{
  auto names = repo.GetContractNames();

  for (const auto & name : names)
  {
    printf("%s\n", name.c_str());
  }

  return 0;
}


int Show(bdcontract::ContractRepository & repo, const std::string & name)
{
  auto contract = repo.LoadContract(name.c_str());
  if (!contract)
  {
    printf("Error: contract not found.\n");
    return -2;
  }

  printf("Name:     %s\n", contract->Name().c_str());
  printf("Provider: %s\n", contract->Provider().c_str());
  printf("Size:     %" PRIu64 " MB\n", contract->Size() / (1024 * 1024));
  printf("Reputation: %" PRIu32 "\n", contract->Reputation());

  return 0;
}


int Delete(bdcontract::ContractRepository & repo, const std::string & name)
{
  repo.DeleteContract(name.c_str());
  return 0;
}


int main(int argc, const char ** argv)
{
  bdcontract::Options::Init(argc, argv);

  bdcontract::ContractRepository repo(bdcontract::Options::repo.c_str());

  if (bdcontract::Options::action == "create")
  {
    return Create(repo,
                  bdcontract::Options::name,
                  bdcontract::Options::provider,
                  bdcontract::Options::size,
                  bdcontract::Options::reputation);
  }
  else if (bdcontract::Options::action == "list")
  {
    return List(repo);
  }
  else if (bdcontract::Options::action == "show")
  {
    return Show(repo, bdcontract::Options::name);
  }
  else if (bdcontract::Options::action == "delete")
  {
    return Delete(repo, bdcontract::Options::name);
  }

  return -1;
}
