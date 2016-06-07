#pragma once

#include <stdint.h>

typedef struct Board{
	uint64_t b[2];
} Board;

typedef struct Move{
	int32_t pos,stone;
} Move;

#define ORDER 0
#define CHAOS 1
#define OO 0
#define XX 1


Board* makeboard(void);
void applymove(Board *board,Move mv);
bool isempty(const Board *board,int pos);
void printboard(const Board *board);
int checkwin(const Board *board);
Move calcmove(Board *board,int player);
