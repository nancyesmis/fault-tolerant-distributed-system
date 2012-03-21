#ifndef LANKE_UTIL_H
#define LANKE_UTIL_H

#include <string>
#include <netdb.h>
#include <stdlib.h>
#include <iostream>
#include <arpa/inet.h>
#include <vector>
#include <ifaddrs.h>

struct kv739_server
{
    std::string hostname;
    int port;
    int ping_port;
    int recover_port;
    int partition_port;
    bool dead;
    bool isrecover;
};

/*
 * Store the data in the key, value map
 */
struct KValue
{
    KValue()
    {
         value = "";
	 time = 0;
    }
    KValue(const std::string& v, long int t)
    {
        value = v;
	time = t;
    }
    KValue(const KValue& copy)
    {
        value = copy.value;
	time = copy.time;
    }
    std::string value;
    long int time;
};

std::string NOVALUE = "[]";
const char* C_NOVALUE = "[]";
int TIME_BASE = 1.33e9;
/*
 * Get ip address from host name
 */
std::string getIp(const std::string& name)
{
    hostent* remote = gethostbyname( name.c_str() );
    if ( remote == NULL )
	return "";
    in_addr* addr = (in_addr* ) remote->h_addr;
    std::string ip = inet_ntoa( *addr );
    return ip;
}

/*
 * Get local IP address
 */
std::vector<std::string>* getLocalIp()
{

    char name[51];
    struct ifaddrs* ifaddr, *ifa;
    std::vector<std::string>* ips = new std::vector<std::string>();
    if ( getifaddrs( &ifaddr ) == -1 )
    {
	std::cout << "getifaddrs error" << std::endl;
	return NULL;
    }
    for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next )
    {
	if ( ifa->ifa_addr == NULL )
	    continue;
	int family = ifa->ifa_addr->sa_family;
	if ( family == AF_INET )
	{
	    int ret = getnameinfo( ifa->ifa_addr, sizeof( struct sockaddr_in ), name, NI_MAXHOST,
	    NULL, 0, NI_NUMERICHOST);
	    ips->push_back(name);
	    std::cout << name << std::endl;
	}
    }
    return ips;
/*    char name[51];
    std::vector<std::string>* ips = new std::vector<std::string>();
    if ( gethostname( name, 50 ) == -1 )
    {
	std::cout << "gethostname error" << std::endl;
	return ips;
    }
    struct hostent *he;
    if ( ( he = gethostbyname(name) ) == NULL )
    {
	std::cout << "gethostbyname error" << std::endl;
	return ips;
    }
    struct in_addr** addr_list = ( struct in_addr** )he->h_addr_list;
    for( int i = 0; addr_list[i] != NULL; i++ )
    {
	ips->push_back( inet_ntoa(*addr_list[i]) );
	std::cout << inet_ntoa(*addr_list[i]) << std::endl;
    }
    return ips;
    */
}

/*
 * Extract key, value from message
 */
void getKeyValue( const std::string& message, std::vector<std::string>& key, std::vector<std::string>& value)
{
    size_t head = 0;
    size_t index1 = 0; 
    size_t index2 = 0; 
    for ( size_t i = 0; i < message.size(); i++ )
    {
	if ( message[i] == '[' )
	{
	    index1 = i;
	}
	else if ( message[i] == ']' )
	{
	    index2 = i;
	    key.push_back( message.substr( head, index1 - head ) );
	    value.push_back( message.substr( index1 + 1, index2 - index1 - 1 ) );
	    head = index2 + 1;
	}
    }
}

/*
 * Extract key, value, time from message
 */
void getKeyValueTime( const std::string& message, std::vector<std::string>& key, std::vector<std::string>& value, std::vector<long int>& time)
{
    size_t head = 0;
    size_t index1 = 0;
    size_t index2 = 0;
    size_t index3 = 0;
    for( size_t i = 0; i < message.size(); i++ )
    {
	if ( message[i] == '[' )
	{
	    index1 = i;
	}
	else if ( message[i] == ']' )
	{
	    if ( index2 > index3 )
	    {
		index3 = i;
		key.push_back( message.substr( head, index1 - head ) );
		value.push_back( message.substr( index1 + 1, index2 - index1 - 1 ) );
		time.push_back( atol( message.substr( index2 + 1, index3 - index2 - 1).c_str() ) );
		head = index3 + 1;
		//std::cout << head << ':' << index1 << ':' << index2 << ':' << index3 << std::endl;
	    }
	    else
	    {
		index2 = i;
	    }
	}
    }
}

class Bufque
{

private:
    std::string* data;
    int head;
    int tail;
    const int SIZE;
public:
    Bufque():SIZE(10000)
    {
	head = 0;
	tail = 0;
	data = new std::string[SIZE];
    }
    std::string front()
    {
	if ( empty() )
	{
	    std::cout << "queue empty " << std::endl;
	    return "";
	}
	return data[ head ];
    }
    void pop()
    {
	head = ( head + 1 ) % SIZE;
    }
    bool push ( const std::string& msg )
    {
	if ( full () )
	{
	    std::cout << "queue full " << std::endl;
	    return false;
	}
	data[tail] = msg;
        tail = ( tail + 1 ) % SIZE;
    }
    bool empty()
    {
	return head == tail;
    }
    bool full()
    {
	return  ( tail + 1 ) % SIZE == head;
    }
    int size()
    {
	int ret = tail - head;
	if ( ret < 0 )
	    ret += SIZE;
	return ret;
    }
};

#endif
