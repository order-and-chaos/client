#pragma once

#include <stdbool.h>

struct Logwidget;
typedef struct Logwidget Logwidget;

Logwidget* lgw_make(int x,int y,int w,int h,const char *title); //title may be "" or NULL for no title; is copied
void lgw_destroy(Logwidget *lgw);
void lgw_redraw(Logwidget *lgw); //called automatically; should only be needed if something else overwrote the widget
void lgw_add(Logwidget *lgw,const char *line);
void lgw_addf(Logwidget *lgw,const char *format,...) __attribute__((format (printf, 2,3)));
void lgw_clear(Logwidget *lgw);


struct Promptwidget;
typedef struct Promptwidget Promptwidget;

Promptwidget *prw_make(int x,int y,int w);
void prw_destroy(Promptwidget *prw);
void prw_redraw(Promptwidget *prw); //should only be needed if overwritten
char* prw_handlekey(Promptwidget *prw,int key); //newly allocated input string if enter, NULL otherwise


typedef struct Menuitem{
	char *text;
	int hotkey;
	void (*func)(void); //will be called by menu_handlekey()
} Menuitem;

typedef struct Menudata{
	int nitems;
	Menuitem *items;
} Menudata;

struct Menuwidget;
typedef struct Menuwidget Menuwidget;

typedef enum Menukey{
	MENUKEY_HANDLED,
	MENUKEY_IGNORED,
	MENUKEY_QUIT,
	MENUKEY_CALLED
} Menukey;

Menuwidget* menu_make(int basex,int basey,const Menudata *data); //keeps a reference to the data!
void menu_destroy(Menuwidget *mw);
void menu_redraw(Menuwidget *mw); //should only be needed if overwritten
Menukey menu_handlekey(Menuwidget *mw,int key);


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
