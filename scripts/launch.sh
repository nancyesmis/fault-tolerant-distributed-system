#!/bin/sh
source servers.sh

for i in ${servers[@]}; do
    echo $i

    ssh -i id_rsa user739@$i "cd p1; ./server 8080 >> server.log" &
done
