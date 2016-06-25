#pragma once

struct Logwidget;
typedef struct Logwidget Logwidget;

Logwidget* lgw_make(int x,int y,int w,int h);
void lgw_destroy(Logwidget *lgw);
void lgw_add(Logwidget *lgw,const char *line);
void lgw_clear(Logwidget *lgw);
