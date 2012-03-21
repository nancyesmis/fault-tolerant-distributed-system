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
	int server;
	if(argc!=5 && argc!=4)
	{
		cout<<"usage: ./rw r/w server key [value]"<<endl;
		exit(1);
	}
	server=atoi(argv[2]);
    char* oldvalue = new char[2500];
    char* value = new char[2000];
    char* key = new char[200];
    int port = 12345;
    init();
    kv739_init( servers );

	if(strcmp(argv[1], "r")==0)
	{
		strcpy(key, argv[3]);
		for(int i=0;i<4;i++)
		{
			if(i==server) continue;
			kv739_fail(servers[i]);
		}
		kv739_get(key, value);
		for(int i=0;i<4;i++)
		{

			if(i==server)continue;
			kv739_recover(servers[i]);
		}
		cout<<key<<"\t"<<value<<endl;
	}
	else
	{
		strcpy(key, argv[3]);
		strcpy(value,argv[4]);
		for(int i=0;i<4;i++)
		{
			if(i==server) continue;
			kv739_fail(servers[i]);
		}
		kv739_put(key, value,oldvalue);
		for(int i=0;i<4;i++)
		{

			if(i==server)continue;
			kv739_recover(servers[i]);
		}
		cout<<key<<"\t"<<value<<"\t"<<oldvalue<<endl;
			
	}
	return 0;

}
