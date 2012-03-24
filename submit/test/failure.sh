./stop.sh
./launch.sh
sleep 2
rm -f failure.txt
source servers.sh
./failure > failure.txt &
sleep 1
./serverOnOff.sh 3 off
sleep 1
./serverOnOff.sh 3 on
./stop.sh
