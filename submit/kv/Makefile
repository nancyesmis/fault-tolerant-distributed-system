CC = g++
#FLAGS= -g
all: server client kv739.so 
server: Socket.o Server.cpp util.h
	$(CC) $(FLAGS) -o server Server.cpp Socket.o -pthread
client: Socket.o client.o kv739.so
	$(CC) $(FLAGS) -o client Client.o Socket.o -L. -l739kv
client.o: Socket.o Client.cpp util.h
	$(CC) $(FLAGS) -c Client.cpp
Socket.o: Socket.cpp Socket.h
	$(CC) $(FLAGS) -c Socket.cpp -pthread
kv739.so: 739kv.o
	$(CC) $(FLAGS) -shared 739kv.o -o lib739kv.so
739kv.o: 739kv.cpp 739kv.h util.h
	$(CC) $(FLAGS) -c -fPIC 739kv.cpp
clean:
	rm -f client server *.o lib739kv.so config.txt servers.sh

