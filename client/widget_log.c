#define _GNU_SOURCE  // vasprintf
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "widget.h"
#include "termio.h"
#include "circbuf.h"
#include "util.h"

struct Logwidget{
	Circbuf *cb;
	int nrows;
	int x,y,w,h;
	int nexty;
};


static void lgw_drawborder(Logwidget *lgw){
	moveto(lgw->x,lgw->y);
	tputc('+');
	for(int i=1;i<lgw->w-1;i++)tputc('-');
	tputc('+');
	for(int i=1;i<lgw->h-1;i++){
		moveto(lgw->x,lgw->y+i);
		tputc('|');
		moveto(lgw->x+lgw->w-1,lgw->y+i);
		tputc('|');
	}
	moveto(lgw->x,lgw->y+lgw->h-1);
	tputc('+');
	for(int i=1;i<lgw->w-1;i++)tputc('-');
	tputc('+');
}

Logwidget* lgw_make(int x,int y,int w,int h){
	assert(x>=0&&y>=0);
	assert(w>=3&&h>=3);
	Logwidget *lgw=malloc(sizeof(Logwidget));
	if(!lgw)return NULL;
	lgw->cb=cb_make(h-2,free);
	if(!lgw->cb){
		free(lgw);
		return NULL;
	}
	lgw->nrows=h-2;
	lgw->x=x;
	lgw->y=y;
	lgw->w=w;
	lgw->h=h;
	lgw->nexty=0;

	pushcursor();
	lgw_drawborder(lgw);
	popcursor();

	return lgw;
}

void lgw_redraw(Logwidget *lgw){
	pushcursor();
	lgw_drawborder(lgw);

	int len=cb_length(lgw->cb);
	int i;
	for(i=0;i<len;i++){
		moveto(lgw->x+1,lgw->y+1+i);
		char *line=cb_get(lgw->cb,i);
		tprintf("%s",line);
		for(int j=strlen(line);j<lgw->w-2;j++)tputc(' ');
	}
	for(;i<lgw->nrows;i++){
		moveto(lgw->x+1,lgw->y+1+i);
		for(int j=0;j<lgw->w-2;j++)tputc(' ');
	}
	popcursor();
}

void lgw_add(Logwidget *lgw,const char *line){
	int len=strlen(line);
	while(len>0){
		int copylen=lgw->w-2;
		if(copylen>len)copylen=len;
		char *copy=malloc(copylen+1);
		assert(copy);
		memcpy(copy,line,copylen);
		copy[copylen]='\0';
		cb_append(lgw->cb,copy);
		len-=copylen;
		line+=copylen;
	}
	lgw_redraw(lgw);
}

__attribute__((format (printf, 2,3))) void lgw_addf(Logwidget *lgw,const char *format,...){
	char *str;
	va_list ap;
	va_start(ap,format);
	vasprintf(&str,format,ap);
	va_end(ap);
	if(!str)outofmem();
	lgw_add(lgw,str);
	free(str);
}

void lgw_clear(Logwidget *lgw){
	cb_clear(lgw->cb);
	lgw_redraw(lgw);
}
