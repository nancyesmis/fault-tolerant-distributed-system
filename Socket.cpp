/*
 * Implement the Socket interface
 *
 */


#include "Socket.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <cstdlib>

using namespace std;


Socket::Socket() :
  m_sock ( -1 ), m_token(']'), m_maxToken(2300)
{

  memset ( &m_addr,
	   0,
	   sizeof ( m_addr ) );
}

Socket::~Socket()
{
  if ( valid() )
  {
      ::close ( m_sock );
  }
  m_sock = -1;
}

void Socket::close()
{
   if( valid() )
       ::close ( m_sock );
   m_sock = -1;
}

bool Socket::buildServer( int port )
{
    if ( ! init() )
    {
        cerr << "Can't initialize socket" << endl;
	close();
	return false;
    }
    if ( ! bind(port) )
    {
        cerr << "Can't bind to port " << port << endl;
	close();
	return false;
    }
    if ( ! listen() )
    {
	cerr << "Can't listen to socket" << endl;
	close();
	return false;
    }
    return true;
}

bool Socket::setTimeout( int sec, int type)
{
// 1 recv, 2 send, 3 both
  struct timeval tv;
  tv.tv_sec = sec;
  tv.tv_usec = 0;
  if ( type % 2 == 1 )
  {
    if (setsockopt( m_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv)) 
    {
      return false;
    }
  }
  if ( type >= 2 )
  {
    if (setsockopt( m_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,  sizeof tv)) 
    {
      return false;
    } 
  }
  return true;
}

bool Socket::init()
{
  m_sock = socket ( AF_INET,
		    SOCK_STREAM,
		    0 );

  if ( m_sock == -1 )
    return false;
  setBlocking( true );


  // TIME_WAIT - argh
  int on = 1;
  if ( setsockopt ( m_sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
    return false;
  return true;

}



bool Socket::bind ( const int& port )
{

  if ( ! valid() )
    {
      return false;
    }

  m_addr.sin_family = AF_INET;
  m_addr.sin_addr.s_addr = INADDR_ANY;
  m_addr.sin_port = htons ( port );

  int bind_return = ::bind ( m_sock,
			     ( struct sockaddr * ) &m_addr,
			     sizeof ( m_addr ) );

  if ( bind_return == -1 )
    {
      return false;
    }

  return true;
}

bool Socket::listen() const
{
  if ( ! valid() )
    {
      return false;
    }

  int listen_return = ::listen ( m_sock, MAXCONNECTIONS );


  if ( listen_return == -1 )
    {
      return false;
    }

  return true;
}


bool Socket::accept ( Socket& new_socket ) const
{
  int addr_length = sizeof ( m_addr );
  new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &m_addr, ( socklen_t * ) &addr_length );

  if ( new_socket.m_sock <= 0 )
    return false;
  else
    return true;
}


bool Socket::send ( const std::string& s ) const
{
  int status = ::send ( m_sock, s.c_str(), s.size(), MSG_NOSIGNAL );
  if ( status == -1 )
    {
      return false;
    }
  else
    {
      return true;
    }
}

bool Socket::recvMessage( std::string& s) const
{
    char buf[ MAXBUFFER + 1 ];
    memset ( buf, 0, MAXBUFFER + 1);
    int num = recv (buf, MAXBUFFER + 1);
    int len = num;
    s = "";
    while ( num > 0 && buf[len- 1] != m_token ) 
    {
	num = recv (buf + len, MAXBUFFER + 1 - len);
	len += num;
    }
    if ( num == -1 || len == 0)
    {
	return false;
    }
    s = buf;
    if ( buf[ len - 1] != m_token )
    {
	return false;
    }
    return true;
}

int Socket::countChar( char* buf)
{
    int count = 0;
    while ( *buf != 0 )
    {
        if ( *buf++ == m_token )
            count ++;
    }
    return count;
}

int Socket::getSetNum( char* numbuf, int numsize )
{
    for ( int i = 0; i < numsize; i++ )
    {
        if ( numbuf[ i ] == m_token )
	{
	    numbuf[ i ] = 0;
	    return i + 1;
	}
    }
}

char* Socket::recvAll( )
{
    const int numsize = 20;
    char numbuf[ numsize ];
    memset( numbuf, 0, numsize );
    int num = recv( numbuf, numsize - 1 );
    int tindex = getSetNum( numbuf, numsize );
    int numToken = atoi ( numbuf + 1 );
    if ( numToken == 0 )
    {
	char* ret = new char[1];
	ret[0] = 0;
	return ret;
    }
    //cout << numbuf << endl;
    //cout << numToken << endl;
    //cout << tindex << endl;
    //cout << strlen( numbuf + tindex ) << endl;
    //cout << numbuf << endl;
    int max = numToken * m_maxToken;
    //cout << "max: " << max << endl;
    char* buf = new char[ max ];
    memset ( buf, 0, max );
    if ( tindex < numsize )
    {
        strcpy ( buf, numbuf + tindex );
    }	
    int len = strlen( numbuf + tindex );
    num = recv ( buf + len, max - len );
    len += num;
    int countToken = countChar( buf );
    while ( countToken < numToken )
    {
	//cout << countToken << ';'  << numToken << endl;
        num = recv ( buf + len, max - len );
	//cout << " each chunk : " << num << endl;
	countToken += countChar( buf + len );
	len += num;
	if ( num <= 0 )
	    break;
    }
    if ( num == -1 || len == 0 || buf[ len - 1] != m_token )
    {
	delete buf;
	return NULL;
    }
    return buf;
}
int Socket::recv ( char* buf, int max) const
{

  int status = ::recv ( m_sock, buf, max, 0);

  if ( status == -1 )
    {
      //std::cout << "Receive error: " << errno << " in Socket::recv\n";
    }
  return status;
}

std::string Socket::getForeignAddr()
{
    return inet_ntoa(m_addr.sin_addr);
}

bool Socket::connect ( const std::string& host, const int port )
{
  if ( ! init() ) return false;

  m_addr.sin_family = AF_INET;
  m_addr.sin_port = htons ( port );

  int status = inet_pton ( AF_INET, host.c_str(), &m_addr.sin_addr );

  if ( errno == EAFNOSUPPORT ) return false;

  status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

  if ( status == 0 )
    return true;
  else
    return false;
}

void Socket::setBlocking ( bool istrue )
{

  istrue = !istrue;
  int opts;

  opts = fcntl ( m_sock,
		 F_GETFL );

  if ( opts < 0 )
    {
      return;
    }

  if ( istrue )
    opts = ( opts | O_NONBLOCK );
  else
    opts = ( opts & ~O_NONBLOCK );

  fcntl ( m_sock,
	  F_SETFL,opts );

}
