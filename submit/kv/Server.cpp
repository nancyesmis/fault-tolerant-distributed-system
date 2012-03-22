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
#include <queue>

#define SERVER_PORT 12345
#define MAXSERVER 4
using namespace std;

map<string, KValue> database;
int server_id = 0;
int num_server = 0;
kv739_server server_list[ MAXSERVER ];
Socket* pgsocks[ MAXSERVER ];

pthread_rwlock_t mutex = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t pgmutex[ MAXSERVER ];

pthread_t waitThreads[ MAXSERVER ];
pthread_t sendThreads[ MAXSERVER ];
pthread_t recoverThreads[ MAXSERVER ];
pthread_t partitionThreads[ MAXSERVER ];
pthread_t checkThread;
pthread_t debugThread;
pthread_t pingThread;

queue<string>  bufmsg[ MAXSERVER ];

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
	if ( clone.size() > 0 )
	    ret = sock.send( ss.str() );
	else
	    ret = sock.send( "[0]" );
	if ( ! ret )
	{
	    cout << "Sending recover data restart " << endl;
	}
	ret = sock.recvMessage( msg );
	if ( ! ret ) 
	{
	    cout << "Receving ack from recover restart" << endl;
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
	   cout << "creaing waiting status " << endl;
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
    vector<string> keys;
    vector<string> values;
    vector<long int> curtimes;
    while ( true )
    {
	server.accept( sock );
	while ( true )
	{
	    bool ret = sock.recvMessage( message );
	    if ( ! ret )
	    {
		cout << " Propagate receive restart" << endl;
		break;
	    }
	    keys.clear();
	    values.clear();
	    curtimes.clear();
	    getKeyValueTime( message, keys, values, curtimes );
	    pthread_rwlock_wrlock( &mutex );
	    for ( int i = 0; i < keys.size(); i++ )
	    {
		string& key = keys[i];
		string& value = values[i];
		long int& curtime = curtimes[i];
    		bool update = false;
		if ( !(database.find( key ) != database.end()
		   && curtime < database[ key ].time) )
		{
		    database[ key ].value = value;
		    database[ key ].time = curtime;
		    update = true;
		}
		if ( update )
		{
		    checkRecoverPropagate( message, curtime, index );
		}
	    }
	    pthread_rwlock_unlock( &mutex);

	}
	sock.close();
    }
}

void checkRecoverPropagate(const string& msg, const long int timecount, const int id)
{
    for ( int i = 0; i < num_server; i++ )
    {
        if ( i != server_id && ( server_list[ i ].isrecover || server_list[ i ].dead ) )
	{
	    pthread_rwlock_wrlock( &pgmutex[i] );
	    bufmsg[i].push( msg );
	    pthread_rwlock_unlock( &pgmutex[i] );
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
	   cout << "creaing waiting status " << endl;
   } 
}

void init()
{
    ifstream iff;
    iff.open( "config.txt" );
    if ( iff.fail() )
    {
        cout << "Open config.txt error" << endl;
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
	//server_list[ lnum ].hostname = line.substr(0, index);
	if ( server_list[ lnum ].hostname.size() == 0 )
	    cout << "Incorrect host name " << endl;
	server_list[ lnum ].cport = atoi( line.substr( index + 1, line.size() - index - 1 ).c_str());
	server_list[ lnum ].port = server_list[ lnum ].cport + 111;
	server_list[ lnum ].ping_port = server_list[ lnum ].port + 221;
	server_list[ lnum ].recover_port = server_list [ lnum ].port + 331;
	server_list[ lnum ].partition_port = server_list [ lnum ].port + 441;
	server_list[ lnum ].dead = false;
	server_list[ lnum ].isrecover = false;

	lnum ++;
    }
    for ( int i = 0; i < MAXSERVER; i++ )
    {
	pgsocks[i] = NULL;
	pthread_rwlock_init( &pgmutex[i], NULL);
    }
    num_server = lnum;
    iff.close();
}

bool  propagateUpdate(const string& msg, long int id )
{
   Socket* client;
   kv739_server* server = &(server_list [ id ]);
   bool ret = false;
   int checkNum = 0;
   if ( pgsocks[id] != NULL )
   {
       client = pgsocks[ id ];
   }
   else
   {
       client = new Socket();
       while ( !( ret = client->connect( server->hostname, server_list[ server_id ].port ) ) )
       {
           client->close();
	   usleep(100);
           if ( ++checkNum > check_fail )
	       break;
       }
       if ( ! ret )
       {
           server_list[ id ].dead = true;
           cout << "Server " << id << " is dead"  << endl;
           client->close();
           return false;
       }
       else
       {
           pgsocks[ id ] = client;
       }
   }
   if ( ! client->setTimeout(1, 3) )
       cout << "set timeout propagate" << endl;
   checkNum = 0;
   if (  ! client->setTimeout(1, 3) )
	cout << "propagate timeout restart " << endl;
   while ( !( ret = client->send( msg ) ) )
   {
       client->close();
       usleep(100);
       if ( ++checkNum > check_fail )
	   break;
   }
   if ( ! ret )
   {
       server_list[ id ].dead = true;
       delete pgsocks[ id ];
       pgsocks[ id ] = NULL;
       cout << "Server  " << id << " is dead" << endl;
       return false;
   }
   return true;
}

void* propagateConsumer( void * index )
{
    long id = (long)index;
    bool ret = false;
    while ( true )
    {
	while ( server_list[id].dead )
	{
	    if ( pingServer(id ) )
	    {
		server_list[id].dead = false;
		cout << "server " << id << " alive " << endl;
		break;
	    }
	    usleep(1000);
	}
	pthread_rwlock_wrlock( & pgmutex[id] );
	if ( bufmsg[id].size() > 0 )
	{
	    ret = propagateUpdate( bufmsg[id].front() , id );
	    if ( ret )
	    {
	        bufmsg[id].pop(); 
	    }
	}
	pthread_rwlock_unlock( & pgmutex[id] );
	usleep(100);
    }	
}

long int getCount()
{
    struct timeval cur_time;
    gettimeofday( & cur_time, NULL );
    return ( cur_time.tv_sec - TIME_BASE ) * 1000 + cur_time.tv_usec / 1000;
}

void addPropagate( const string& key, const string& value, long int timecount )
{
    stringstream ss;
    ss << key << '[' << value << ']'  << timecount << ']';
    for ( int i = 0; i < num_server; i++ )
    {
	if ( i == server_id )
	    continue;
        pthread_rwlock_wrlock( &pgmutex[i] );
	bufmsg[i].push( ss.str() );
	pthread_rwlock_unlock( &pgmutex[i] );	
    }
}

void waitUntilAll(int iter )
{
    int index = 0;
    while ( true )
    {
	bool end = true;
	for ( int i = 0; i < num_server; i++ )
	{
	    if ( i == server_id )
		continue;
	    if ( bufmsg[i].size() > 0 && ! server_list[i].dead )
	    {
		end = false;
		break;
	    }
	}
	usleep(100);
	//cout << "waiting " << endl;
	index++;
	if ( index > iter )
	    return;
    }
}

void start()
{
    map<string, KValue>::iterator iter;
    Socket server;
    server.buildServer( server_list[ server_id ].cport );
    string NOVALUE = "[]";
    Socket sock;
    int num = 0;
    string message;
    vector<string> keys;
    vector<string> values;
    stringstream ss;
    while ( true )
    {
	server.accept ( sock );
	sock.setTimeout(1, 2);
	while ( true )
	{
		bool suc = sock.recvMessage( message );
		cout << message << endl;
		if ( ! suc )
		{
		    cout << "Receiving restart " << endl;
		    break;
		}
		//cout << message << endl;
		keys.clear();
		values.clear();
		bool propagated = false;
		getKeyValue(message, keys, values);
		ss.clear();
		for( int i = 0; i < keys.size(); i ++ )
		{
		    string& key = keys[i];
		    string& value = values[i];
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
		    if ( value.size() != 0 )
		    {
			database[ key ].value = value;
			database[ key ].time = timecount;
		    }
		    pthread_rwlock_unlock( &mutex );
		    if ( value.size() != 0 )
		    {
			addPropagate( key, value, timecount);
		    }
		    ss >> message;
		    //waitUntilAll(10);
		    usleep(100);
		    suc = sock.send ( message );
		    if ( ! suc )
		    {
			cout << " Server send restart" << endl;
			sock.close();
			break;
		    }
		}
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
    recoverDatabase( data, false );
    bool ret = client->send( msg );
    if ( ! ret )
    {
        cout << "Sending recover ack" << endl;
    }
    cout << "Recovered from server " << id << endl;
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
    cout << database.size() << endl;
    while ( pos != NULL )
    {
        pch1 = strchr( pos, '[' );
	*pch1 = 0;
	pch2 = strchr( pch1 + 1, '[' );
        *pch2 = 0;
	timecount = atol( pch2 + 1 );
	if ( timecount >= database[ pos ].time )
	{
	    database [ pos ].value = pch1 + 1;
	    database [ pos ].time = timecount;
	}
	pos = strtok( NULL, "]" );
    }
    pthread_rwlock_unlock( &mutex );
    delete data;
    return 0;
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
	    cout << "Connecting recover server " << endl;
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

void startThreads( pthread_t* ts, void * (func) (void * ) )
{
   for( int i = 0; i < num_server; i++ )
   {
       if ( server_id == i )
	   continue;
       int status = pthread_create( & ts[ i ], NULL, func, (void* )i);
       if ( status != 0 )
	   cout << "creaing waiting status" << endl;
   }
}

int getServerId( )
{
    bool notFound = true;
    vector<string>* ips = getLocalIp();
    int id = -1;
    for( int i = 0; i < ips->size() && notFound; i ++ )
    {
        for ( int j = 0; j < num_server && notFound; j++ )
	{
	    if ( ips->at(i).compare( server_list[j].hostname ) == 0 )
	    {
		id = j;
		notFound = false;
	    }
	}
    }
    delete ips;
    return id;
}

int main(int argc, char** argv)
{
    init();
    if ( argc > 1 )
    {
	server_id = atoi(argv[1]);
	if ( server_id < 0 || server_id >= num_server )
	{
	    cout << " server id error  " << endl;
	    exit(-1);
	}
    }
    else
    {
	server_id = getServerId();
    }
    if ( server_id < 0 )
    {
	cout << "server id error" << endl;
	exit(-1);
    }
    startThreads( waitThreads, waitUpdate);
    if ( strcmp(argv[argc - 1], "recover") == 0 )
    {
	while ( ! recover() )
	    sleep ( 1 );
    }
    pthread_create( &debugThread, NULL, debugFunction, NULL );
    pthread_create( &pingThread, NULL, waitPing, NULL );
    startThreads( recoverThreads, waitRecover );
    startThreads( sendThreads, propagateConsumer );
    start();
}