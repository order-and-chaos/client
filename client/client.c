#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <assert.h>

#include "termio.h"
#include "singleplayer.h"
#include "multiplayer.h"
#include "util.h"


extern const char *serverhost;

static Menuitem mainmenuitems[]={
	{"Multiplayer online",'m',startmultiplayer},
	{"Single player versus AI",'s',startsingleplayer},
	{"Quit",'q',NULL}
};
static Menudata mainmenudata={3,mainmenuitems};

int main(int argc,char **argv){
	if(argc==2){
		serverhost=argv[1];
	}

	initkeyboard();
	atexit(endkeyboard);
	initscreen();
	atexit(endscreen);

	installCLhandler(true);

	moveto(0,0);
	setbold(true);
	tprintf("Order & Chaos");
	setbold(false);

	Menuwidget *mw=menu_make(2,2,&mainmenudata);
	if(!mw)outofmem();
	bool shouldquit=false;
	while(!shouldquit){
		redraw();
		int key=tgetkey();
		Menukey ret=menu_handlekey(mw,key);
		switch(ret){
			case MENUKEY_HANDLED:
				break;

			case MENUKEY_IGNORED:
				bel();
				break;

			case MENUKEY_QUIT:
				shouldquit=true;
				break;

			case MENUKEY_CALLED:
				clearscreen();
				moveto(0,0);
				setbold(true);
				tprintf("Order & Chaos");
				setbold(false);
				menu_redraw(mw);
				break;
		}
	}
	menu_destroy(mw);
}
