#include "Socket.h"
#include "739kv.h"
#include "util.h"
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <sys/time.h>

using namespace std;


char* servers[4];
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
    memset( oldvalue, 0, 200 );
    memset( value , 0, 2000 ); 
    init();
    kv739_init( servers );
    strcpy( value, "value" );
    strcpy( key, "key" );
    kv739_put( key, value, oldvalue );
    cout << oldvalue << ":" << value << endl;
    kv739_get( key, value );
    cout << value << endl;
}
