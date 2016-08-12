#!/bin/bash
set -x
# random ints between 00-FF
OCT1=$(openssl rand -hex 1) 
OCT2=$(openssl rand -hex 1) 

rm -rf /tmp/stdout
rm -rf /tmp/stderr
rm -rf /tmp/finish

for i in 1 2 4;
    do
    CS=$[$i-1]
    if [ "$CS" == "0" ]; then
	CS="$[0]"
    else
	CS="0-${CS}"
    fi
    taskset -c $CS qemu-system-x86_64 -cpu host -enable-kvm -serial stdio -display none -m 4G -smp cpus=$i --netdev tap,id=vlan1,if
name=virttap,script=no,downscript=no,vhost=on,queues=2 --device virtio-net-pci,mq=on,vectors=6,netdev=vlan1,mac=02:00:00:04:$OCT1:$
OCT2 -kernel /tmp/hoardthreadtest/Release/bm/hoardthreadtest.elf32 -append 'niterations=1000;nobjects=100000;nthreads='$i';work=0;o
bjSize=8;' >>/tmp/stdout 2>>/tmp/stderr; date >>/tmp/finish;
done

