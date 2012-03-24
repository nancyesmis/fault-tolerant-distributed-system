./stop.sh
./launch.sh
sleep 2
rm -f stress_partition.txt
./consistency > stress_partition.txt &
sleep 2
./dopartition.sh part
sleep 1
./dopartition.sh unpart
#./eventual
./stop.sh
