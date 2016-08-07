#pragma once

#include <stdbool.h>

struct Boardwidget;
typedef struct Boardwidget Boardwidget;

typedef enum Boardkey{
	BOARDKEY_HANDLED,
	BOARDKEY_IGNORED
} Boardkey;

Boardwidget* bdw_make(int basex,int basey,bool colorcursor);
void bdw_destroy(Boardwidget *bdw);
void bdw_redraw(Boardwidget *bdw); //should only be needed if overwritten
Boardkey bdw_handlekey(Boardwidget *bdw,int key);
