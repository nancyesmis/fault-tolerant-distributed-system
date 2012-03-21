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
	strcpy( servers[index], line.c_str() );
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
    memset( oldvalue, 0, 200 );
    memset( value , 0, 2000 ); 
    init();
    kv739_init( servers );
    int index = -1;
    int keyid = 0;
    while ( true )
    {
	index++;
	sprintf(value, "%d", index );
	if ( index % 100 == 0 )
	    sprintf(key, "%s%d", "key", keyid++);
	//cout << index << ". sending " <<  value << endl;
	/*
	for( int i = 0; i < 4; i++ )
	{
	    if ( i == index % 4 )
		kv739_recover( servers[i] );
	    else
		kv739_fail( servers[i] );	    
	}
	*/
	//kv739_recover( servers[ (index + 1) % 2] );
	kv739_put( key, value, oldvalue );
	if ( atoi(value ) - atoi(oldvalue) != 1 && atoi(value) % 100 != 0)
	{
	    cout << oldvalue << ":" << value << endl;
	    cout << "inconsistency " << endl;
	}
	//usleep(3000);
	//sleep(1);
	//cin >> key >> value;
    }
}
