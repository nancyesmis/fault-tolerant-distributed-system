source servers.sh
for i in 0 1 2 3; do
    ssh -i id_rsa user739@${servers[$i]} "cd ~/kv; rm -f server.log ; ./server $i client > server.log & " &
done
