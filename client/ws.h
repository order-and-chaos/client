#pragma once

#include <stdbool.h>

struct ws_context;
typedef struct ws_context ws_context;

struct ws_conn;
typedef struct ws_conn ws_conn;

ws_context* ws_init(void);
void ws_destroy(ws_context *ctx);
ws_conn* ws_connect(ws_context *ctx,const char *hostname,const char *port,const char *path);
void ws_close(ws_conn *conn);

bool ws_isopen(ws_conn *conn);

void ws_write(ws_conn *conn,const char *data,int length); //blocks until completely sent
void ws_writestr(ws_conn *conn,const char *str); //ws_write, assuming null-terminated
bool ws_write_noblock(ws_conn *conn,const char *data,int length); //returns whether fully written
bool ws_writestr_noblock(ws_conn *conn,const char *str); //ws_write_noblock, assuming null-terminated
bool ws_flush(ws_conn *conn); //returns whether everything flushed

int ws_selectsock(ws_conn *conn);
char* ws_readlinex(ws_conn *conn,char delim); //returns newly allocated null-terminated string, NULL in case of error
char* ws_readline(ws_conn *conn); //readlinex with '\n'

void ws_ping(ws_conn *conn);
