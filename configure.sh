#!/bin/bash

declare -A interfaces=([0]="enp0s3" [1]="enp0s8" [2]="enp0s9")
declare -A addresses


i=0
while [[ -n "$1" ]]; do
  addresses["$i"]=$1
  shift
  ((i++))
done

for ((j=0;j<i;j++)); do
  sudo ip link set up dev "${interfaces[$i]}"
  sudo ip addr add "${addresses[$i]}" dev "${interfaces[$i]}"
done

sudo ip link set up dev enp0s3


