#!/usr/bin/env bash
#161 253
for i in {5100..5250}
do
    sudo ./cmake-build-debug/mymasterproject enp0s8 $i &
done
