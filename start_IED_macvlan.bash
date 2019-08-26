#!/bin/bash
iface="enp0s3";

for i in {5100..5300}
do
    j=$(( $i-5100+1 ));
    mac=$(printf "00:09:8E:00:00:%02X\n" $j);
    #Remove existing v-NIC
    ip link del $iface.$j;
    ip link add link $iface address $mac $iface.$j type macvlan mode bridge
    ifconfig $iface.$j up
    ./cmake-build-debug/mymasterproject $iface.$j $i  &
done

#echo "LIED10 has started\n"

#${OUTPUT_PATH} -p 102 -i lo -z 0x1000 -z 0x1001 -z 0x1002 -x LIED10CTRL/LLN0\$GO\$Status -x LIED10PROT/LLN0\$GO\$Alarm -x LIED10MEAS/LLN0\$GO\$Meas -c 4000
