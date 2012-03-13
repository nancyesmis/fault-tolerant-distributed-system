#ifndef LANKE_UTIL_H
#define LANKE_UTIL_H

#include <string>
#include <netdb.h>
#include <stdlib.h>
#include <iostream>
#include <arpa/inet.h>
#include <vector>

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
    }
    return ips;
}

/*
 * Extract key, value from message
 */
void getKeyValue( const std::string& message, std::string& key, std::string& value)
{
    size_t index = message.find('[');
    key = message.substr(0, index);
    value = message.substr( index + 1, message.size() - index - 2);
}

/*
 * Extract key, value, time from message
 */
void getKeyValueTime( const std::string& message, std::string& key, std::string& value, long int& time)
{
    size_t index = message.find('[');
    key = message.substr(0, index);
    size_t second = message.find(']');
    value = message.substr( index + 1, second - index - 1);
    time = atol(message.substr( second + 1, message.size() - second - 2 ).c_str() );
}

#endif
