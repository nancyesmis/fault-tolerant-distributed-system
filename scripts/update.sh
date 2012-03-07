#!/bin/sh
source servers.sh
echo ${servers}
for i in 0 1 2 3; do
    echo ${servers[$i]}:8080 >& servers.txt
    for j in 0 1 2 3; do
	if [ "$i" -ne "$j" ]
	then
	  echo ${servers[$j]}:8080 >> servers.txt
	fi
    done
    ssh user739@${servers[$i]} "mkdir p1"
    scp servers.txt user739@${servers[$i]}:p1
    scp *.[ch] Makefile user739@${servers[$i]}:p1
    ssh user739@${servers[$i]} "cd p1 ; make" 
done
