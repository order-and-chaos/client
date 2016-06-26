#include <stddef.h>

#include "menu.h"
#include "termio.h"

void show_menu(int basex,int basey,const Menudata *data){
	int choice=0;
	while(true){
		clearscreen();

		data->drawback();
		
		for(int i=0;i<data->nitems;i++){
			moveto(basex,basey+i);
			tprintf("> %s",data->items[i].text);
			if(data->items[i].hotkey!='\0'){
				tprintf(" (");
				setbold(true);
				tputc(data->items[i].hotkey);
				setbold(false);
				tputc(')');
			}
		}
		moveto(basex,basey+choice);
		redraw();

		bool restartmenu=false;
		while(!restartmenu){
			int key=getkey();
			switch(key){
				case 'j': case KEY_DOWN:
					if(choice<data->nitems-1){
						choice++;
						moveto(basex,basey+choice);
						redraw();
					} else bel();
					break;

				case 'k': case KEY_UP:
					if(choice>0){
						choice--;
						moveto(basex,basey+choice);
						redraw();
					} else bel();
					break;

				case '\n':
					if(data->items[choice].func!=NULL){
						data->items[choice].func();
						restartmenu=true;
					} else return;
					break;

				default: {
					int i;
					for(i=0;i<data->nitems;i++){
						if(data->items[i].hotkey==key)break;
					}
					if(i==data->nitems)bel();
					else {
						if(data->items[i].func==NULL)return;
						data->items[i].func();
						restartmenu=true;
					}
					break;
				}
			}
		}
	}
}
