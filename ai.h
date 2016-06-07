#pragma once

#include <stdint.h>

// Game board representation
typedef struct Board{
	uint64_t b[2];
} Board;

// Single move representation
typedef struct Move{
	int32_t pos,stone;
} Move;

// Players and stone types
#define ORDER 0
#define CHAOS 1
#define OO 0
#define XX 1

// Construct a Board
Board* makeboard(void);

// Apply the given move to the board (doesn't matter by whom)
void applymove(Board *board,Move mv);

// Test whether the specified position is empty
bool isempty(const Board *board,int pos);

// Print the board to stdout in a very simple format; useful for debugging
void printboard(const Board *board);

// Check whether someone has won on the board
// -1: nobody won yet; ORDER: Order has won; CHAOS: Chaos has won
int checkwin(const Board *board);

// Calculate a "good" move for the given player on the board (doesn't apply yet)
Move calcmove(Board *board,int player);
