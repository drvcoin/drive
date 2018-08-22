#! /bin/bash

mkdir -p contracts

id=0;

while [[ $id -lt 8 ]]; do

  let port=$id+3000
  let reputation=$id+3

  ./bdcontract create "contract-$id" "host-$id" 256 $reputation -r contracts

  let id=$id+1

done
