CC = g++
FLAGS= -g
all: server client kv739.so throughput consistency failure rw 
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
	rm -f client server *.o lib739kv.so
throughput.o: Socket.o throughput.cpp util.h
	$(CC) $(FLAGS) -c throughput.cpp
consistency.o: Socket.o consistency.cpp util.h
	$(CC) $(FLAGS) -c consistency.cpp
failure.o: Socket.o failure.cpp util.h
	$(CC) $(FLAGS) -c failure.cpp
rw.o: Socket.o rw.cpp util.h
	$(CC) $(FLAGS) -c rw.cpp
throughput : Socket.o throughput.o kv739.so
	$(CC) $(FLAGS) -o throughput throughput.o Socket.o -L. -l739kv
consistency: Socket.o consistency.o kv739.so
	$(CC) $(FLAGS) -o consistency consistency.o Socket.o -L. -l739kv
failure: Socket.o failure.o kv739.so
	$(CC) $(FLAGS) -o failure failure.o Socket.o -L. -l739kv
rw: Socket.o rw.o kv739.so
	$(CC) $(FLAGS) -o rw rw.o Socket.o -L. -l739kv

