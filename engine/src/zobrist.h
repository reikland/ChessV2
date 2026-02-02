#pragma once
#include "types.h"

extern uint64_t Z_PIECE[12][64];
extern uint64_t Z_CASTLE[16];
extern uint64_t Z_EPFILE[9];
extern uint64_t Z_SIDE;

void zobrist_init(uint64_t seed);
