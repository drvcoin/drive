#! /bin/bash

mkdir -p root
mkdir -p node1
mkdir -p node2
mkdir -p node3

\cp default_contacts.json root/
\cp default_contacts.json node1/
\cp default_contacts.json node2/
\cp default_contacts.json node3/

./bdkad -p 7800 -k root -n 127.0.0.1 4000 &
sleep 2
./bdkad -p 7801 -k node1 -n 127.0.0.1 4001 &
sleep 1
./bdkad -p 7802 -k node2 -n 127.0.0.1 4002 &
sleep 1
./bdkad -p 7803 -k node3 -n 127.0.0.1 4003 &
sleep 1
