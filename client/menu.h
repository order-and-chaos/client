#pragma once

typedef struct Menuitem{
	char *text;
	int hotkey;
	void (*func)(void); //NULL to just return
} Menuitem;

typedef struct Menudata{
	int nitems;
	Menuitem *items;
	void (*drawback)(void); //should redraw everything else, so show_menu can clear the screen upon reshowing menu
} Menudata;

void show_menu(int basex,int basey,const Menudata *data);
