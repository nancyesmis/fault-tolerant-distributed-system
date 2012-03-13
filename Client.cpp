#include "Socket.h"
#include "739kv.h"
#include "util.h"
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <fstream>
using namespace std;

char* servers[4];
const char* server_port = "12345";
void init()
{
    ifstream iff;
    iff.open( "config.txt" );
    if ( iff.fail() )
    {
	cout << "open file error" << endl;
	return;
    }
    string line;
    int index = 0;
    while ( iff >> line )
    {
	servers[index] = new char[50];
        size_t pos = line.find(':');
	strcpy( servers[index], line.substr(0, pos + 1).c_str() );
	strcat( servers[index], server_port );
	index++;
    }
    servers[ index ] = NULL;
    iff.close();
}


int main(int argc, char** argv)
{
    char* oldvalue = new char[2500];
    char* value = new char[2000];
    char* key = new char[200];
    int port = 12345;
    cout << sizeof(oldvalue) << endl;
    memset( oldvalue, 0, sizeof(oldvalue) );
    memset( value , 0, sizeof( value ) ); 
    init();
    kv739_init( servers );
    int index = 0;
    while ( true )
    {
	cin >> key >> value;
	index++;
	kv739_put( key, value, oldvalue );
	cout << oldvalue << endl;
	kv739_get( key, oldvalue );
	cout << oldvalue << endl;
        cout << index << endl;
	sleep(1);
    }
}
