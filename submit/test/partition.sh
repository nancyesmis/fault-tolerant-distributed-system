./stop.sh
./launch.sh
sleep 2
echo write to server0 [1,2]
./rw w 0 1 2
echo read from server1 and server2
./rw r 1 1
./rw r 2 1
echo create partition
./dopartition.sh part
sleep 2
echo write to partition1
./rw w 0 1 3
echo read from partition1
./rw r 0 1
./rw r 1 1
echo read from partition2
./rw r 2 1
./rw r 3 1
echo recover from partition
./dopartition.sh unpart
echo 'start'
#sleep 10
echo 'end'
echo read all servers
for (( i=0; i<=1000; i++ ))
do 
echo $i
./rw r 0 1
./rw r 1 1
./rw r 2 1
./rw r 3 1
sleep 0.1 
done
./stop.sh
