#pragma once
#include "types.h"

typedef uint32_t Move;

#define M_FROM(m)   ((int)((m) & 63u))
#define M_TO(m)     ((int)(((m) >> 6) & 63u))
#define M_PIECE(m)  ((Piece)(((m) >> 12) & 15u))
#define M_CAP(m)    ((Piece)(((m) >> 16) & 15u))
#define M_PROMO(m)  ((Piece)(((m) >> 20) & 15u))
#define M_FLAGS(m)  ((uint32_t)((m) >> 24))

static inline Move move_encode(int from, int to, Piece pc, Piece cap, Piece promo, uint32_t flags) {
    return (Move)((from & 63)
        | ((to & 63) << 6)
        | ((pc & 15) << 12)
        | ((cap & 15) << 16)
        | ((promo & 15) << 20)
        | ((flags & 255u) << 24));
}
