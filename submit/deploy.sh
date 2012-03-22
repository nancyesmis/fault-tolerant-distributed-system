#!/bin/sh
source servers.sh
echo ${servers}
for i in 0 1 2 3; do
    echo ${servers[$i]}:${ports[$i]} >> config.txt
done
cp config.txt kv
cp servers.sh kv
tar -cvf  kv.tar kv
for i in ${servers[@]}; do
        scp kv.tar user739@$i:~
        ssh user739@$i "cd ~ ; tar -xvf kv.tar; cd kv ; make clean; make "
done
cd kv 
make
cd ..
