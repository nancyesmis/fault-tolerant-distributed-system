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
	int count=500;
	vector<pair<string, string> > keyValueSequence[40];
    char* oldvalue = new char[2500];
    char* value = new char[2000];
    char* key = new char[200];
    int port = 12345;
    cout << sizeof(oldvalue) << endl;
    memset( oldvalue, 0, sizeof(oldvalue) );
    memset( value , 0, sizeof( value ) ); 
    init();
    kv739_init( servers );

	int server=0;
//	for(int i=0;i<count;i++)
	for(int j=0;j<keyNum;j++)
	{
//		for(int j=0;j<keyNum;j++)
		for(int i=0;i<count;i++)
		{
			// write to servers[server]
			for (int k=0;k<4;k++)
			{
				if (k==server) continue;
				kv739_fail(servers[k]);
			}
                        cout << server << endl;
			sprintf(key, "%d", j);
			sprintf(value, "%d", rand());
			if( kv739_put(key, value, oldvalue) ==-1 ) exit(1);
			string v=value ;
			string o=oldvalue ;
			for (int k=0;k<4;k++)
			{
				if(k==server)continue;
				kv739_recover(servers[k]);
			}
			pair<string, string> p(o, v);
			keyValueSequence[j].push_back(p);
			server++;
			server%=4;
			usleep(100);
		}
	}
	for(int i=0;i<keyNum;i++)
	{
		cout<<i<<":";
		for(int j=0;j<count-1;j++)
		{
			cout<<"["<<keyValueSequence[i][j].first<<","<<keyValueSequence[i][j].second<<"]";
			if( keyValueSequence[i][j].second!=keyValueSequence[i][j+1].first) 
			{
				cout<<"\n inconsistency \n";
//				cout<<"["<<keyValueSequence[i][j+1].first<<","<<keyValueSequence[i][j+1].second<<"]";
//				exit(1);
			}
		}
		
		cout<<"["<<keyValueSequence[i][count-1].first<<","<<keyValueSequence[i][count-1].second<<"]";
		cout<<endl;
	}	
	return 0;

}
