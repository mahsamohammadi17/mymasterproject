#!/usr/bin/env bash

for i in {5161..5251}
do
    sudo ./cmake-build-debug/mymasterproject enp0s8 $i &
done
