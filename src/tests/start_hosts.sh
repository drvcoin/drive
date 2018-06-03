#! /bin/bash

id=0;

while [[ $id -lt 8 ]]; do

  let port=$id+3000

  mkdir -p host-$id

  pushd host-$id > /dev/null

  ../bdhost -p $port -r ../contracts &

  popd > /dev/null

  let id=$id+1

done
