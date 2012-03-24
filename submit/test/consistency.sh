./stop.sh
./launch.sh
sleep 2
rm -f consistency.txt
./consistency > consistency.txt &
