#include "Socket.h"
#include "Server.h"
#include "util.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <list>
#include <pthread.h>
#include <sys/time.h>
#include <climits>

#define SERVER_PORT 12345
#define MAXSERVER 4
using namespace std;

map<string, KValue> database;
int server_id = 0;
int num_server = 0;
kv739_server server_list[ MAXSERVER ];

pthread_rwlock_t mutex = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t recent_mutex = PTHREAD_RWLOCK_INITIALIZER;

pthread_t waitThreads[ MAXSERVER ];
pthread_t sendThreads[ MAXSERVER ];
pthread_t recoverThreads[ MAXSERVER ];
pthread_t partitionThreads[ MAXSERVER ];
pthread_t checkThread;
pthread_t debugThread;
pthread_t pingThread;

long int last_msg[ MAXSERVER ];

//store the recent write message
list<KValue*> recent_msg;


void* waitRecover( void* id )
{
    long index = (long)id;
    Socket server;
    bool ret = server.buildServer( server_list[ index ].recover_port );
    if ( ! ret )
    {
        cout << "Creating recover server error" << endl;
	return NULL;
    }
    Socket sock;
    string msg, key, value;
    while ( true )
    {
	server.accept( sock );
	
	server_list[ index ].dead = false;
        server_list[ index ].isrecover = true;
	pthread_rwlock_rdlock( &mutex );
	map<string, KValue> clone( database );
	pthread_rwlock_unlock( &mutex );
	map<string, KValue>::iterator liter;
	
	cout << "recover request from " << index << endl;
	stringstream ss;
	ss << '[' << clone.size() << ']';
	for ( liter = clone.begin(); liter != clone.end(); liter ++ )
	{
	    ss << liter->first << '[' << liter->second.value << '[' << liter->second.time << ']';
	}
	cout << "sending data " << endl;
	if ( clone.size() > 0 )
	    ret = sock.send( ss.str() );
	else
	    ret = sock.send( "[0]" );
	cout << ss.str() << endl;
	if ( ! ret )
	{
	    cout << "Sending recover data error " << endl;
	}
	ret = sock.recvMessage( msg );
	cout << "received ack" << endl;
	if ( ! ret ) 
	{
	    cout << "Receving ack from recover error " << endl;
	}
	else
	{
	    last_msg[ index ] = getCount();
	}
	server_list[ index ].isrecover = false;
	sock.close();
    }
}

void startRecoverServer()
{
    for ( int i = 0; i < num_server; i++ )
    {
        if ( i == server_id )
	    continue;
        int status = pthread_create( &recoverThreads[ i ], NULL, waitRecover, (void* )i );
        if ( status != 0 )
	   cout << "creaing waiting status error " << endl;
    }
}

void* waitPing( void* arg )
{
      Socket server;
      server.buildServer( server_list[ server_id ].ping_port );
      Socket sock;
      while ( true )
      {
          server.accept( sock );
          sock.close();
      }
}

void* waitUpdate(void* id)
{
    long index = (long) id;
    Socket server;
    server.buildServer ( server_list[ index ].port );
    Socket sock;
    string message;
    string key;
    string value;
    long int curtime;
    while ( true )
    {
	server.accept( sock );
        sock.recvMessage( message );
	getKeyValueTime( message, key, value , curtime);
	cout << "message received: " << message  << endl;
	pthread_rwlock_wrlock( &mutex );
	bool update = false;
	if ( !(database.find( key ) != database.end()
		&& curtime < database[ key ].time) )
	{
	    database[ key ].value = value;
	    database[ key ].time = curtime;
	    update = true;
	}
	pthread_rwlock_unlock( &mutex);
	if ( update )
	{
	    pthread_rwlock_wrlock( &recent_mutex );
	    recent_msg.push_back( new KValue( key, curtime ) );
	    pthread_rwlock_unlock( &recent_mutex );
	    checkRecoverPropagate( message, curtime, index );
	}
	sock.close();
    }
}

void checkRecoverPropagate(const string& msg, const long int timecount, const int id)
{
    for ( int i = 0; i < num_server; i++ )
    {
        if ( i != server_id && server_list[ i ].isrecover )
	{
	    thread_arg* arg = new thread_arg();
	    arg->message = msg;
	    arg->id = id;
	    arg->timecount = timecount;
	    int status = pthread_create( &sendThreads[id], NULL, propagateUpdate, arg );
	    if ( status != 0 )
		cout << " creating recover forward thread error" << endl;
	}
    }
}

void startServerUpdates()
{
   for( int i = 0; i < num_server; i++ )
   {
       if ( server_id == i )
	   continue;
       int status = pthread_create( &waitThreads[ i ], NULL, waitUpdate, (void* )i);
       if ( status != 0 )
	   cout << "creaing waiting status error " << endl;
   } 
}

void init()
{
    ifstream iff;
    iff.open( "config.txt" );
    if ( iff.fail() )
    {
        cout << "Open config.txt error " << endl;
	exit(-1);
    }
    string line;
    int lnum = 0;
    while ( iff >> line && lnum < MAXSERVER)
    {
	size_t index = line.find(':');
	if ( index == string::npos )
        {
	    cout << "Incorrect config format host:port " << endl;
	    exit(-1);
	}
	server_list[ lnum ].hostname = getIp( line.substr( 0, index ) );
	if ( server_list[ lnum ].hostname.size() == 0 )
	    cout << "Incorrect host name " << endl;
	server_list[ lnum ].port = atoi( line.substr( index + 1, line.size() - index - 1 ).c_str());
	server_list[ lnum ].ping_port = server_list[ lnum ].port + 10;
	server_list[ lnum ].recover_port = server_list [ lnum ].port + 20;
	server_list[ lnum ].partition_port = server_list [ lnum ].port + 30;
	server_list[ lnum ].dead = false;
	server_list[ lnum ].isrecover = false;

	lnum ++;
    }
    num_server = lnum;
    iff.close();
}

void* propagateUpdate(void * argument )
{
   thread_arg* arg = (thread_arg* )argument;
   Socket client;
   kv739_server* server = &(server_list [ arg->id ]);
   bool ret = false;
   int checkNum = 0;
   while ( !( ret = client.connect( server->hostname, server_list[ server_id ].port ) ) )
   {
       client.close();
       usleep(1000);
       if ( ++checkNum > check_fail )
	   break;
   }
   if ( ! ret )
   {
       server_list[ arg->id ].dead = true;
       cout << "Server " << arg->id << " is dead"  << endl;
       client.close();
       delete arg;
       return NULL;
   }
   checkNum = 0;
   while ( !( ret = client.send( arg->message ) ) )
   {
       client.close();
       usleep(1000);
       if ( ++checkNum > check_fail )
	   break;
   }
   if ( ! ret )
   {
       server_list[ arg->id ].dead = true;
       cout << "Server  " << arg->id << " is dead" << endl;
       client.close();
       delete arg;
       return NULL;
   }
   last_msg[ arg->id ] = arg->timecount;
   client.close();
   delete arg;
}

void startPropagateUpdate(const string & message, long int timecount)
{
    stringstream ss;
    ss << message << timecount << ']';
    for ( int i = 0; i < num_server; i++ )
    {
	if ( i != server_id && ! server_list[i].dead )
	{
	    thread_arg* arg = new thread_arg();
	    arg->message = ss.str();
	    arg->id = i;
	    arg->timecount = timecount;
	    int status = pthread_create( &sendThreads[i], NULL, propagateUpdate, arg);
	    if ( status != 0 )
		cout << "creating sending thread error " << endl;
	}
    }
}

long int getCount()
{
    struct timeval cur_time;
    gettimeofday( & cur_time, NULL );
    return ( cur_time.tv_sec - TIME_BASE ) * 1000 + cur_time.tv_usec / 1000;
}
void start()
{
    map<string, KValue>::iterator iter;
    Socket server;
    server.buildServer( SERVER_PORT );
    string NOVALUE = "[]";
    Socket sock;
    int num = 0;
    string message;
    string key;
    string value;
    stringstream ss;
    while ( true )
    {
	server.accept ( sock );
	bool suc = sock.recvMessage( message );
        if ( ! suc )
	{
	    cout << "Receiving error " << endl;
	    continue;
	}
	cout << message << ':' << endl;
	getKeyValue(message, key, value);
	ss.clear();
	long int timecount = getCount();
	pthread_rwlock_wrlock( &mutex );
	if ( database.find( key ) != database.end() )
	{
	    ss << '[' << database[ key ].value << ']';
	}
	else
	{
	    ss << "[[]]" << endl;
	}
	if ( value.compare( NOVALUE ) != 0 )
	{
	    database[ key ].value = value;
	    database[ key ].time = timecount;
	}
	pthread_rwlock_unlock( &mutex );
	if ( value.compare( NOVALUE ) != 0 )
	{
	    pthread_rwlock_wrlock( & recent_mutex );
	    recent_msg.push_back( new KValue( key, timecount ) );
            pthread_rwlock_unlock( & recent_mutex );
	    startPropagateUpdate( message , timecount);
	}
	ss >> message;
	suc = sock.send ( message );
	sock.close();
	if ( ! suc )
	{
	    cout << " Server send error " << endl;
	    continue;
	}
	for ( iter = database.begin(); iter != database.end(); iter ++ )
	{
	    cout << iter->first << ':' << iter->second.value << ':' << iter->second.time<< endl;
	}

    }
}

bool recover()
{
    int id;
    Socket* client = chooseRecoverServer( id );
    string msg = "[ok]";
    if ( client == NULL )
    {
        cout << "No other server is alive " << endl;
	delete client;
	return true;
    }
    char* data = client->recvAll();
    if ( data == NULL )
    {
	delete client;
	return false;
    }
    cout << data << endl;
    recoverDatabase( data, false );
    bool ret = client->send( msg );
    if ( ! ret )
    {
        cout << "Sending recover ack error" << endl;
    }
    delete client;
    return true;
}

long int recoverDatabase( char* data, bool ispartition)
{
    char* pos = strtok( data, "]" );
    char* pch1 = NULL;
    char* pch2 = NULL;
    long int timecount = 0;
    pthread_rwlock_wrlock( &mutex );
    if ( ispartition ) 
    {
	pthread_rwlock_wrlock( &recent_mutex );
    }
    while ( pos != NULL )
    {
        pch1 = strchr( pos, '[' );
	*pch1 = 0;
	pch2 = strchr( pch1 + 1, '[' );
        *pch2 = 0;
	timecount = atol( pch2 + 1 );
	if ( timecount > database[ pos ].time )
	{
	    database [ pos ].value = pch1 + 1;
	    database [ pos ].time = timecount;
	    if ( ispartition )
	    {
	        recent_msg.push_back( new KValue( pos, timecount ) );		
	    }
	}
	pos = strtok( NULL, "]" );
    }
    if ( ispartition )
    {
	pthread_rwlock_unlock( &recent_mutex );
    }
    pthread_rwlock_unlock( &mutex );
    delete data;
    return timecount;
}

bool pingServer( int id )
{
    bool ret = false;
    Socket ping;
    if ( ping.connect( server_list[ id ].hostname, server_list[ id ].ping_port ) )
	ret = true;
    ping.close();
    return ret;
}

Socket* chooseRecoverServer( int& id )
{
    Socket* client = new Socket();
    int checked = 0;
    int index = -1;
    for ( int i = 0; i < num_server; i++ )
    {
	if ( i == server_id )
	    continue;
	if ( ! pingServer(i) )
	    server_list[i].dead = true;
	else if ( index == -1 )
	    index = i;
    }
    if ( index == - 1 )
    {
        cout << "No server is available" << endl;
	delete client;
	return NULL;
    }
    else
    {
	id = index;
        bool ret = client->connect( server_list[index].hostname, server_list[ server_id ].recover_port );
	if ( ! ret )
	{
	    cout << "Connecting recover server error " << endl;
	    delete client;
	    return NULL;
	}
	return client;
    }
}

void* debugFunction( void* arg )
{
    char com;
    while ( cin >> com )
    {
        switch ( com )
	{
	    case 'l': 
		{
		    map<string, KValue>::iterator titer;
		    for ( titer = database.begin(); titer != database.end(); titer++ )
		    {
			cout << titer->first << ':' << titer->second.value<< ':' << titer->second.time << endl;
		    }
		    break;
		}
	    default:
		{
		    cout << database.size() << endl;
		}
	}
    }
}

void* waitPartition( void * id )
{
    long index = (long)id;
    Socket server;
    //port for reconcile partition...
    server.buildServer( server_list[ index ].partition_port );
    Socket sock;
    stringstream ss;
    while ( true )
    {
        bool ret = server.accept( sock );
	if ( ! ret )
	    cout << "partition accept error " << endl;
	char * data = sock.recvAll();
	if ( data == NULL )
	{
	    cout << "No data needs to be udpated from partition" << endl;
	    sock.close();
	    continue;
	}
	cout << data << endl;
	recoverDatabase( data, true );
	sock.close();
    }
}

void* recoverPartition( void * arg )
{
    while ( true )
    {
	bool allGood = true;
	for ( int i = 0; i < num_server; i++ )
	{
	    if ( i == server_id )
		continue;
	    if ( server_list [ i ].dead )
		allGood = false;
	    if ( server_list[ i ].dead && pingServer( i ) )
	    {
		Socket client;
		bool ret = client.connect( server_list[ i ].hostname, server_list [ server_id ].partition_port );
		if ( ! ret )
		{
		    cout << "error: ping ok, connect not " << endl;
		    client.close();
		    continue;
		}
                stringstream ss;
		list< KValue* >::iterator liter;

		server_list[ i ].dead = false;
		pthread_rwlock_rdlock( &mutex );
		pthread_rwlock_rdlock( &recent_mutex );
		
		map<string, KValue> clone( database );
		list<KValue* > rclone( recent_msg );
		
		pthread_rwlock_unlock( &recent_mutex );
		pthread_rwlock_unlock( &mutex );
		
                int tokenCount = 0;
		bool started = false;
		for ( liter = rclone.begin(); liter != rclone.end(); liter++ )
		{
		    if ( (*liter)->time > last_msg[ i ] )
		    {
			if ( ! started )
			    ss << '[' << rclone.size() - tokenCount << ']';
			started = true;
		        ss << (*liter)->value << '[' << clone[ (*liter)->value ].value << '[' << (*liter)->time << ']';
		    }
		    tokenCount++;
		}
		if ( started )
		    ret = client.send( ss.str() );
		else
		    ret = client.send( "[0]" );
		cout << "partition sent " << ss.str() << endl;
		if ( ! ret )
		{
		    cout << " Partition send error" << endl;
		    server_list[ i ].dead = true;
		}
		if ( ret && started )
		{
		    last_msg[ i ] = getCount();
		}
		client.close();
	    }
	}
	if ( allGood )
	{
	    pthread_rwlock_wrlock( & recent_mutex );
	    while ( recent_msg.size() > 0 )
	    {
	        delete recent_msg.front();
		recent_msg.pop_front();
	    }
	    pthread_rwlock_unlock( & recent_mutex );
	}
	sleep( check_period );
    }
}

void startThreads( pthread_t* ts, void * (func) (void * ) )
{
   for( int i = 0; i < num_server; i++ )
   {
       if ( server_id == i )
	   continue;
       int status = pthread_create( & ts[ i ], NULL, func, (void* )i);
       if ( status != 0 )
	   cout << "creaing waiting status error " << endl;
   }
}

int main(int argc, char** argv)
{
    if ( argc < 2 )
    {
	cout << "Usage: server id" << endl;
	exit(-1);
    }
    server_id = atoi( argv[1] ) - 1;
    if ( server_id < 0 || server_id > 3)
    {
        cout << "Id should be 1 - " << MAXSERVER << endl;
	exit(-1);
    }
    init();
    startThreads( waitThreads, waitUpdate);
    if ( argc == 3 && strcmp(argv[2], "recover") == 0 )
    {
	while ( ! recover() )
	    sleep ( 1 );
    }
    pthread_create( &debugThread, NULL, debugFunction, NULL );
    pthread_create( &pingThread, NULL, waitPing, NULL );
    pthread_create( &checkThread, NULL, recoverPartition, NULL );
    startThreads( partitionThreads, waitPartition );
    startThreads( recoverThreads, waitRecover );
    start();
}
