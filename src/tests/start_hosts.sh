#! /bin/bash

id=0;

while [[ $id -lt 8 ]]; do

  let port=$id+3000

  mkdir -p host-$id

  pushd host-$id > /dev/null

  hostIP=`ip addr | grep 'state UP' -A2 | tail -n1 | awk '{print $2}' | cut -f1  -d'/'`

  ../bdhost -p $port -n host-$id -e "http://$hostIP:$port" -k "http://localhost:7800" -c "../contracts/contract-$id" &

  popd > /dev/null

  let id=$id+1

done
