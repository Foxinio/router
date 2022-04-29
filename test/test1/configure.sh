#!/bin/bash

if [[ "$1" == 1 ]]; then
  sudo ip link set up dev enp0s3
  sudo ip addr add 172.16.1.13/16 dev enp0s3
elif [[ "$1" == 2 ]]; then
  sudo ip link set up dev enp0s3
  sudo ip addr add 172.16.1.14/16 dev enp0s3
  sudo ip link set up dev enp0s8
  sudo ip addr add 192.168.2.10/24 dev enp0s8
  sudo ip link set up dev enp0s9
  sudo ip addr add 10.0.1.2/8 dev enp0s9
elif [[ "$1" == 3 ]]; then
  sudo ip link set up dev enp0s3
  sudo ip addr add 192.168.2.5/24 dev enp0s3
  sudo ip link set up dev enp0s8
  sudo ip addr add 192.168.5.5/24 dev enp0s8
elif [[ "$1" == 4 ]]; then
  sudo ip link set up dev enp0s3
  sudo ip addr add 192.168.5.43/24 dev enp0s3
  sudo ip link set up dev enp0s8
  sudo ip addr add 10.0.1.1/8 dev enp0s8
fi
