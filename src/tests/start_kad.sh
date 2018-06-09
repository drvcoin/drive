#! /bin/bash

./bdkad -p 7800 -k root -n 127.0.0.1 4000 -b 127.0.0.1 4000 &
sleep 1
./bdkad -p 7801 -k node1 -n 127.0.0.1 4001 -b 127.0.0.1 4000 &
sleep 1
./bdkad -p 7802 -k node2 -n 127.0.0.1 4002 -b 127.0.0.1 4000 &
sleep 1
./bdkad -p 7803 -k node3 -n 127.0.0.1 4003 -b 127.0.0.1 4000 &
sleep 1
