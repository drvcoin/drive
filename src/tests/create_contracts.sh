#! /bin/bash

mkdir -p contracts

id=0;

while [[ $id -lt 8 ]]; do

  let port=$id+3000

  ./bdcontract create "contract-$id" test "host-$id" 256 -r contracts

  let id=$id+1

done
