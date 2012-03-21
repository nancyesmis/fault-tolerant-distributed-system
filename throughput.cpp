#include "Socket.h"
#include "739kv.h"
#include "util.h"
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <fstream>

#include <sstream>
#include <cstdlib>
#include <time.h>

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
	if(argc!=2 && argc!=3){
		cout<<"usage: ./throughput key# count#/key [start]"<<endl;
		exit(1);
	}
	int keyNum=atoi(argv[1]);
	int count=atoi(argv[2]);
//	cout<<keyNum<<"\t"<<count<<endl;	
	int start=0;
	if (argc==4)
	{
		start=atoi(argv[3]);
	}

    char* oldvalue = new char[2500];
    char* value = new char[2000];
    char* key = new char[200];
    int port = 12345;
//    cout << sizeof(oldvalue) << endl;
    memset( oldvalue, 0, sizeof(oldvalue) );
    memset( value , 0, sizeof( value ) ); 
    init();
    kv739_init( servers );
    int index = 0;

//    srand(time(0));

    for (int i=0;i<count;i++)
    {
	for (int j=0; j<keyNum;j++)
	{
		stringstream ss;
		ss<<(j+start);
		ss>>key;
		ss<<rand();
		ss>>value;
	//	cout<<key<<"\t"<<value<<endl;
		kv739_put(key, value, oldvalue);			
	} 

    }
    return 0;
}
