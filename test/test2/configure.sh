#!/bin/bash

if [[ "$1" == 1 ]]; then
  sudo ip link set up dev enp0s3
  sudo ip addr add 192.168.1.1/24 dev enp0s3
  sudo ip link set up dev enp0s8
  sudo ip addr add 192.168.4.1/24 dev enp0s8
elif [[ "$1" == 2 ]]; then
  sudo ip link set up dev enp0s3
  sudo ip addr add 192.168.1.2/24 dev enp0s3
  sudo ip link set up dev enp0s8
  sudo ip addr add 192.168.2.2/24 dev enp0s8
elif [[ "$1" == 3 ]]; then
  sudo ip link set up dev enp0s3
  sudo ip addr add 192.168.2.3/24 dev enp0s3
  sudo ip link set up dev enp0s8
  sudo ip addr add 192.168.3.3/24 dev enp0s8
elif [[ "$1" == 4 ]]; then
  sudo ip link set up dev enp0s3
  sudo ip addr add 192.168.3.4/24 dev enp0s3
  sudo ip link set up dev enp0s8
  sudo ip addr add 192.168.4.4/24 dev enp0s8
fi
