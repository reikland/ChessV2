#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef uint64_t U64;

enum { WHITE = 0, BLACK = 1 };

typedef enum {
    EMPTY = 0,
    WP, WN, WB, WR, WQ, WK,
    BP, BN, BB, BR, BQ, BK
} Piece;

typedef enum {
    FLAG_NONE    = 0,
    FLAG_CAPTURE = 1 << 0,
    FLAG_EP      = 1 << 1,
    FLAG_CASTLE  = 1 << 2,
    FLAG_PROMO   = 1 << 3,
    FLAG_DBLPUSH = 1 << 4
} MoveFlags;

static inline int piece_color(Piece p) {
    if (p >= WP && p <= WK) return WHITE;
    if (p >= BP && p <= BK) return BLACK;
    return -1;
}

static inline int popcount64(U64 x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(x);
#else
    int c = 0;
    while (x) {
        x &= x - 1;
        ++c;
    }
    return c;
#endif
}

static inline int lsb_index(U64 x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(x);
#else
    int i = 0;
    while ((x & 1u) == 0u) {
        x >>= 1;
        ++i;
    }
    return i;
#endif
}

static inline int msb_index(U64 x) {
#if defined(__GNUC__) || defined(__clang__)
    return 63 - __builtin_clzll(x);
#else
    int i = 63;
    while (((x >> i) & 1u) == 0u) {
        --i;
    }
    return i;
#endif
}
