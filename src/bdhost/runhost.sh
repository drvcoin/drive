#! /bin/bash

action=$1
id=0

while [[ $id -lt 8 ]]; do
  name=part$id

  mkdir -p test/$id
  pushd test/$id > /dev/null
  
  if [[ "$action" == "init" ]]; then
    ../../bdhost init $name
  else
    let port=$id+3000
    ../../bdhost listen $port &
  fi

  popd > /dev/null

  let id=$id+1
done
