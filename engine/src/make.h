#pragma once
#include "position.h"

bool make_move(Position *pos, Move mv);
void undo_move(Position *pos, Move mv);
bool in_check(const Position *pos, int side);
