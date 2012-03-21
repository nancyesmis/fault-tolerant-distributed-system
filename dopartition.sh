# ip addresses of servesr
source servers.sh

# put the indexes of the machines in the a and b partitions here
partitiona=(0 1)
partitionb=(2 3)

if [ "$1" = "unpart" ]
then
    cmd=-D
else 
    if [ "$1" = "part" ]
    then
	cmd=-A
    else
	echo "Usage: dopartition.sh [part|unpart]"
	exit
    fi
fi

action=REJECT


# create partition A
for i in ${partitiona[@]}; do
    for j in ${partitionb[@]}; do
	echo user739@${servers[$i]}:"sudo iptables $cmd INPUT -s ${servers[$j]} -j $action"
	ssh -i id_rsa user739@${servers[$i]} "sudo iptables $cmd INPUT -s ${servers[$j]} -j $action" 
    done
done

# create partition b
for i in ${partitionb[@]}; do
    for j in ${partitiona[@]}; do
	echo user739@${servers[$i]}:"sudo iptables $cmd INPUT -s ${servers[$j]} -j $action"
	ssh -i id_rsa user739@@${servers[$i]} "sudo iptables $cmd INPUT -s ${servers[$j]} -j $action" 
    done
done
