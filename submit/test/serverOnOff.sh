source servers.sh
if [ "$2" == "on" ]
then
ssh -i id_rsa user739@${servers[$1]} "cd ~/fault-tolerant-distributed-system; rm -f server.log ; ./server $1  recover client forward > server.log & " &
else
	if [ "$2" == "off" ]
	then
		ssh -i id_rsa user739@${servers[$1]} "killall server" 
	else
		echo "usage ./serverOnOff.sh [servID] [on/off]"
	fi
fi

