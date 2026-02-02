#pragma once
#include "position.h"

#define MAX_MOVES 256

typedef struct {
    Move m[MAX_MOVES];
    int n;
} MoveList;

void gen_pseudo_legal(const Position *pos, MoveList *list);
bool is_legal_move(Position *pos, Move mv);
