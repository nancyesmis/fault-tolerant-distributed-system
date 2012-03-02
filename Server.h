#ifndef LANKE_SERVER_H
#define LANKE_SERVER_H

#include <string>

/*
 * Read the config.txt to get the server list
 */
void init();

/*
 * Start the server to receive messages
 */
void start();

/*
 * Get the key and value from the message
 * @param m, the message
 * @param k, the key to be returned
 * @param v, the value to be returned
 */

void getKeyValue(const std::string& m, std::string& k, std::string& v);

/*
 * start the thread to propagate update
 */
void startPropagateUpdate(const std::string& message);

/*
 * Thread function to send updates to other servers
 */
void* propagateUpdate(void *);

/*
 * thread function wait the updates from other server
 */
void* waitUpdate(void* arg);

/*
 * start the threads to wait for updates from other servers
 */
void startServerUpdates();

/*
 * return the current timeStamp
 */
long int getCount();

/*
 * recover database from char * get from other server
 */
long int  recoverDatabase( char* data, bool ispartition );

/*
 * Check if Forward message to the recovering server is needed
 */
void checkRecoverPropagate(const std::string& msg, const long int timecount,
	const int id );

/*
 * Wait for partition reconcile request
 */
void * waitPartition( void * id );

/*
 * check the server_list periodicaly to discover partition recover
 */
void * recoverPartition( void * arg );

/*
 * respond the ping from other server
 */
void* waitPing( void * arg );

/*
 * ping the server with id
 */
bool pingServer( int id );

/*
 * Debug thread
 */
void* debugFunction( void * arg );
/*
 * Thread struct used to pass the message and server id
 */
struct thread_arg
{
    std::string message;
    int id;
    long int timecount;
};

/*
 * Recover from failure
 */
bool recover();

/*
 * Choose the server to copy the data
 */
Socket* chooseRecoverServer(int &id);
const int check_fail = 3;
const int check_period = 5;
#endif
