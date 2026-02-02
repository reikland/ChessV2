#pragma once
#include "types.h"

extern U64 KNIGHT_ATTACKS[64];
extern U64 KING_ATTACKS[64];
extern U64 PAWN_ATTACKS[2][64];

void tables_init(void);
U64 rook_attacks(int sq, U64 occ);
U64 bishop_attacks(int sq, U64 occ);
