#pragma once

#include <sys/time.h> //struct timeval

#include "ws.h"

typedef struct Message{
	int id;
	char *typ;
	char **args;
	int nargs;
} Message;

//cb is called when response message (with same id) is received; pass NULL as
//cb to not register callback.
//Varargs are the message arguments, and should all be (const) char*
//Returns whether successful.
bool msg_send(ws_conn *conn,const char *typ,void (*cb)(ws_conn *conn,const Message*),int nargs,...);

//populate_fdsets should populate the given fd_set's with any additional fd's
//that should trigger an event, and return a timeout after which timeouthandler
//should be called (or NULL to not give such a timeout). The timeout will be
//freed by msg_runloop.
//msghandler is called when a free message is received.
//fdhandler is called with the fd_set's from select(2) when some fd's (besides
//possibly conn) are set. Use this to listen for other events.
//fdhandler and timeouthandler may be NULL if no such events are requested.
void msg_runloop(
	ws_conn *conn,
	struct timeval* (*populate_fdsets)(fd_set *readfds,fd_set *writefds),
	void (*msghandler)(ws_conn *conn,const Message*),
	void (*fdhandler)(ws_conn *conn,const fd_set*,const fd_set*),
	void (*timeouthandler)(ws_conn *conn));
