#pragma once
#include "types.h"
#include "move.h"

#define MAX_PLY 256

typedef struct {
    uint64_t key;
    int ep_sq;
    uint8_t castle_rights;
    uint8_t halfmove_clock;
    Piece captured;
} State;

typedef struct {
    U64 bb_piece[12];
    U64 bb_color[2];
    U64 occ;

    Piece piece_on[64];

    int side;
    int king_sq[2];

    uint64_t key;
    int ep_sq;
    uint8_t castle_rights;
    uint8_t halfmove_clock;
    int ply;

    State st[MAX_PLY];
} Position;

bool pos_from_fen(Position *pos, const char *fen);
void pos_update_occupancy(Position *pos);
void pos_compute_key(Position *pos);
