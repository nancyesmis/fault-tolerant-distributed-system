#include "Socket.h"
#include "739kv.h"
#include "util.h"
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <fstream>

#include <vector>
#include <utility>
#include <cstdio>
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
	int keyNum=40;
    char* oldvalue = new char[2500];
    char* value = new char[2000];
    char* key = new char[200];
    int port = 12345;
    cout << sizeof(oldvalue) << endl;
    memset( oldvalue, 0, sizeof(oldvalue) );
    memset( value , 0, sizeof( value ) ); 
    init();
    kv739_init( servers );

	for(int i=0;i<keyNum;i++)
	{
		sprintf(key, "%d", i);
		for (int j=0;j<4;j++)
		{
			for (int k=0;k<4;k++)
			{
				if(k==j) continue;
				kv739_fail(servers[k]);
			}
			if(kv739_get(key, value)==1)
			{	
				cout<<"-1\t";
			}
			else
				cout<<value<<"\t";
			for (int k=0;k<4;k++)
			{
				if(k==j) continue;
				kv739_recover(servers[k]);
			}
			
		}
		cout<<endl;
	}
	return 0;

}
