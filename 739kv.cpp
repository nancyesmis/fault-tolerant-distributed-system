#include "739kv.h"
#include "Socket.h"
#include "util.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <cstring>
using namespace std;

struct kv739_server slist[MAXSERVER];
Socket* socks[MAXSERVER];

int snum = 0;

void setSockNull(Socket* sock)
{
    for ( int i = 0; i < MAXSERVER; i++ )
    {
	if ( socks[i] == sock )
	{
	    socks[i] = NULL;
	    delete sock;
	    return;
	}
    }
    cout << "error no sock found " << endl; 
}

int hash_server(char* str)
{
    unsigned int seed = 131;
    unsigned int hash = 0;
    
    while (*str)
    {
	    hash = hash * seed + *str;
	    str++;
    }
 
    return (hash & 0x7FFFFFFF) % snum;    
}

string getValue( const string & feedback )
{
    size_t index = feedback.find('[');
    return feedback.substr( index + 1, feedback.size() - index - 2);
}

Socket* getSocket(char * key)
{
    int index = hash_server( key );
    int checked = 0;
    Socket* client = NULL;
    while ( true )
    {
	checked++;
	if ( checked > snum )
	{
	    cout << "No server is available " << endl;
	    return NULL;	    
	}
	if ( socks[index] != NULL )
	{
	    return socks[index];
	}
	else
	{
	    client = new Socket();
	    bool ret = client->connect(slist[index].hostname, slist[index].port);
	    if ( !ret )
	    {
		delete client;   
	    }
	    else
	    {
		break;
	    }
	}
	index = ( index + 1 ) % snum;
    }

    client->setTimeout(1, 3);
    socks[ index ] = client;
    return client;
}

void kv739_init(char* servers[])
{
    snum = 0;
    for ( int i = 0; i < MAXSERVER; i++ )
    {
	if ( servers[ i ] == NULL )
	    break;
	snum++;
	socks[i] = NULL;
	string temp = servers[i];
	size_t pos = temp.find(':');
	if ( pos == string::npos )
	{
	    cout << "Server format host:port" << endl;
	    cout << temp << endl;
	    return;
	}
	slist[i].hostname = getIp( temp.substr(0, pos) );
	if ( slist[i].hostname.size() == 0 )
	    cout << "Incorrect hostname " << endl;
	slist[i].dead = false;
	temp = temp.substr(pos + 1, temp.size() - pos);
	slist[i].port = atoi(temp.c_str());
    }
    return;
}

void kv739_fail(char* server)
{
    string temp = server;
    size_t pos = temp.find(':');
    if ( pos == string::npos )
    {
	cout << "Server format host:port" << endl << temp << endl;
	return;
    }
    string host = temp.substr(0, pos);
    temp = temp.substr(pos + 1, temp.size() - pos);
    int port = atoi( temp.c_str() );
    for ( int i = 0; i < snum; i++)
    {
	if ( slist[i].hostname.compare(host) == 0 && slist[i].port == port )
	{
	    slist[i].dead = true;
	    return;
	}
    }
    cout << "No server " << server << " found! " << endl;
    return;
}

void kv739_recover(char* server)
{
    string temp = server;
    size_t pos = temp.find(':');
    if ( pos == string::npos )
    {
	cout << "Server format host:port" << endl << temp << endl;
	return;
    }
    string host = temp.substr(0, pos);
    temp = temp.substr(pos + 1, temp.size() - pos);
    int port = atoi( temp.c_str() );
    for ( int i = 0; i < snum; i++)
    {
	if ( slist[i].hostname.compare(host) == 0 && slist[i].port == port )
	{
	    slist[i].dead = false;
	    return;
	}
    }
    cout << "No server " << server << " found! " << endl;
    return;
}

int kv739_get(char* key, char* value)
{
    return kv739_put( key, const_cast<char*>(""), value );
}


int kv739_put(char* key, char* value, char* oldvalue)
{
    //cout << "sending " << key << ':' << value << endl;
    Socket* client = getSocket( key );
    if ( client == NULL )
       return -1;	
    stringstream ss;
    ss << key << '[' << value << ']';
    bool ret = client->send( ss.str() );
    if ( ! ret )
    {
        cout << "Sending restart" << endl;
	setSockNull(client);
	return kv739_put(key, value, oldvalue);
    }
    string feedback;
    ret = client->recvMessage( feedback );
    if ( ! ret )
    {
	cout << feedback << endl;
        cout << "Receiving restart " << endl;
	setSockNull(client);
	return kv739_put(key, value, oldvalue);
    }
    string ov = getValue( feedback );
    if ( ov.compare(NOVALUE) == 0 )
    {
	return 1;
    }
    strcpy( oldvalue, ov.c_str() );
    return 0;
}


