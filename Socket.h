/*
 * Create an socket wrap for the socket API
 */

#ifndef Socket_interface
#define Socket_interface


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>


const int MAXCONNECTIONS = 50000;
const int MAXBUFFER = 2500;

class Socket
{
 public:
  Socket();
  virtual ~Socket();
  
  /*
   * Accept connection request
   */
  bool accept ( Socket& ) const;
  /*
   * Close the socket
   */
  void close();
  
  /*
   * Set the time out of the recv
   * type 1 recv, 2 send, 3 both
   */
  bool setTimeout( int sec, int type );
  /*
   * Initialize for the server socket
   */
  bool buildServer( int port );
  /*
   * Connect to a server as client socket
   */
  bool connect ( const std::string& host, const int port );
  /* 
   * Send data through the socket
   */
  bool send ( const std::string& ) const;
  /*
   * Receive the whole message
   */
  bool recvMessage ( std::string& ) const;
  /*
   * Receive data
   */
  int recv ( char* buf, int max ) const;
  /*
   * Recv All data with maximum size max
   * The data received will start with the number of tokens
   */
  char* recvAll();
  /*
   * default blocking
   */
  void setBlocking ( bool istrue);
  /*
   * Get the address of the peer
   */
  std::string getForeignAddr();
  /*
   * Check whether the socket is valid
   */
  bool valid() const { return m_sock != -1; }

 private:
  /*
   * Create the socket
   */
  bool init();
  /*
   * Bind the socket to the port
   */
  bool bind ( const int& port );
  /*
   * Listen to the socket
   */
  bool listen() const;
  /*
   * The socket handle
   */
  int m_sock;
  /*
   * The socket address used 
   */
  sockaddr_in m_addr;
  /*
   * The delimiter of the tokens
   */
  const char m_token;
  /*
   * The max number of bytes of a token
   */
  const int m_maxToken;

private:
  /*
   * Count the occurrence of m_token
   */
  int countChar( char * buf );
  /*
   * Get the index of m_token and set it to '\0'
   */
  int getSetNum( char* number, int numsize);
};
#endif
