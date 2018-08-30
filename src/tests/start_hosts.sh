#! /bin/bash

id=0;

while [[ $id -lt 8 ]]; do

  let port=$id+3000

  mkdir -p host-$id

  hostIP=`ip addr | grep 'state UP' -A2 | tail -n1 | awk '{print $2}' | cut -f1  -d'/'`

  ./bdhost -p $port -n host-$id -e "http://$hostIP:$port" -k "http://localhost:7800" -w "host-$id" -s 1000000000 &

  let id=$id+1

done
