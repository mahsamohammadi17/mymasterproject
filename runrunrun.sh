#!/usr/bin/env bash

for i in {5161..5251}
do
    sudo ./cmake-build-debug/mymasterproject lo $i &
done
