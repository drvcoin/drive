#! /bin/bash

./bdkad -p 7800 -k root 127.0.0.1 4000 127.0.0.1 4000 &
sleep 1
./bdkad -p 7801 -k node1 127.0.0.1 4001 127.0.0.1 4000 &
sleep 1
./bdkad -p 7802 -k node2 127.0.0.1 4002 127.0.0.1 4000 &
sleep 1
./bdkad -p 7803 -k node3 127.0.0.1 4003 127.0.0.1 4000 &
sleep 1
