#ifndef kv739_h
#define kv739_h

#include "Socket.h"
#include <string>
#define MAXSERVER 4
/*
 * Initialize the server list
 */
void kv739_init(char* servers[] );

/*
 * Set a server to be failed, and not to contact it
 * @param host:port
 */
void kv739_fail(char* server);

/*
 * Recover a server
 * @param host:port
 */
void kv739_recover(char* server);

/*
 * get the value of the corresponding key
 *
 * @return 0 if key exists, 1 otherwise, failure -1
 */
int kv739_get(char* key, char* value);

/*
 * Set the value of a key
 *
 * @param key the new value
 * @param oldvalue the old value is returned
 *
 * @return 0 if success and old value exists, 1 if no old value, -1 failure
 */
int kv739_put(char* key, char* value, char* oldvalue);

//kv739_server kv739_server_list[MAXSERVER];
//int kv739_server_num = 0;
#endif
