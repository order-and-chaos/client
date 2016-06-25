#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "widget.h"
#include "termio.h"

typedef struct Circbuf{
	char **data;
	int sz,len,base;
	void (*deleteitem)(void*);
} Circbuf;


static Circbuf* cb_make(int size,void (*deleteitem)(void*)){
	Circbuf *cb=malloc(sizeof(Circbuf));
	if(!cb)return NULL;
	cb->data=calloc(size,sizeof(char*));
	if(!cb->data){
		free(cb);
		return NULL;
	}
	cb->sz=size;
	cb->len=0;
	cb->base=0;
	cb->deleteitem=deleteitem;
	return cb;
}

static void cb_append(Circbuf *cb,char *item){
	int index=(cb->base+cb->len)%cb->sz;
	if(cb->data[index])cb->deleteitem(cb->data[index]);
	cb->data[index]=item;
	if(cb->len==cb->sz)cb->base=(cb->base+1)%cb->sz;
	else cb->len++;
}

static void cb_clear(Circbuf *cb){
	for(int i=0;i<cb->len;i++){
		cb->deleteitem(cb->data[(cb->base+i)%cb->sz]);
	}
	cb->len=0;
}

static char* cb_get(Circbuf *cb,int index){
	assert(-cb->len<=index&&index<cb->len);
	if(index<0)index+=cb->len;
	return cb->data[(cb->base+index)%cb->sz];
}

static int cb_length(Circbuf *cb){
	return cb->len;
}


struct Logwidget{
	Circbuf *cb;
	int nrows;
	int x,y,w,h;
	int nexty;
};


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
	moveto(x,y);
	tputc('+');
	for(int i=1;i<w-1;i++)tputc('-');
	tputc('+');
	for(int i=1;i<h-1;i++){
		moveto(x,y+i);
		tputc('|');
		moveto(x+w-1,y+i);
		tputc('|');
	}
	moveto(x,y+h-1);
	tputc('+');
	for(int i=1;i<w-1;i++)tputc('-');
	tputc('+');
	popcursor();

	return lgw;
}

static void lgw_redraw(Logwidget *lgw){
	pushcursor();
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

void lgw_clear(Logwidget *lgw){
	cb_clear(lgw->cb);
	lgw_redraw(lgw);
}
