#pragma once

// Size of the board
#define N 6

// Whether to print the board after each move in terminal I/O mode
#define PRINTBOARD

// Whether to enable asserts -- highly recommended, but disabling gives
// a tiny speed boost
#define ASSERTS

// Search depth for minimax-alpha/beta
#define MMAB_MAXDEPTH 5
// #define MMAB_MAXDEPTH 9

// Number of playthroughs for Monte Carlo
#define MC_PLAYTHROUGHS 10000
