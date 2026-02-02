#include "tables.h"

U64 KNIGHT_ATTACKS[64];
U64 KING_ATTACKS[64];
U64 PAWN_ATTACKS[2][64];

static inline int on_board(int file, int rank) {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}

void tables_init(void) {
    for (int sq = 0; sq < 64; ++sq) {
        int file = sq & 7;
        int rank = sq >> 3;
        U64 k = 0;
        int df[8] = {1, 2, 2, 1, -1, -2, -2, -1};
        int dr[8] = {2, 1, -1, -2, -2, -1, 1, 2};
        for (int i = 0; i < 8; ++i) {
            int nf = file + df[i];
            int nr = rank + dr[i];
            if (on_board(nf, nr)) {
                k |= 1ULL << (nr * 8 + nf);
            }
        }
        KNIGHT_ATTACKS[sq] = k;

        U64 king = 0;
        for (int dfk = -1; dfk <= 1; ++dfk) {
            for (int drk = -1; drk <= 1; ++drk) {
                if (dfk == 0 && drk == 0) continue;
                int nf = file + dfk;
                int nr = rank + drk;
                if (on_board(nf, nr)) {
                    king |= 1ULL << (nr * 8 + nf);
                }
            }
        }
        KING_ATTACKS[sq] = king;

        U64 wp = 0, bp = 0;
        if (on_board(file - 1, rank + 1)) wp |= 1ULL << ((rank + 1) * 8 + (file - 1));
        if (on_board(file + 1, rank + 1)) wp |= 1ULL << ((rank + 1) * 8 + (file + 1));
        if (on_board(file - 1, rank - 1)) bp |= 1ULL << ((rank - 1) * 8 + (file - 1));
        if (on_board(file + 1, rank - 1)) bp |= 1ULL << ((rank - 1) * 8 + (file + 1));
        PAWN_ATTACKS[WHITE][sq] = wp;
        PAWN_ATTACKS[BLACK][sq] = bp;
    }
}

U64 rook_attacks(int sq, U64 occ) {
    U64 attacks = 0;
    int file = sq & 7;
    int rank = sq >> 3;
    for (int r = rank + 1; r < 8; ++r) {
        int s = r * 8 + file;
        attacks |= 1ULL << s;
        if (occ & (1ULL << s)) break;
    }
    for (int r = rank - 1; r >= 0; --r) {
        int s = r * 8 + file;
        attacks |= 1ULL << s;
        if (occ & (1ULL << s)) break;
    }
    for (int f = file + 1; f < 8; ++f) {
        int s = rank * 8 + f;
        attacks |= 1ULL << s;
        if (occ & (1ULL << s)) break;
    }
    for (int f = file - 1; f >= 0; --f) {
        int s = rank * 8 + f;
        attacks |= 1ULL << s;
        if (occ & (1ULL << s)) break;
    }
    return attacks;
}

U64 bishop_attacks(int sq, U64 occ) {
    U64 attacks = 0;
    int file = sq & 7;
    int rank = sq >> 3;
    for (int f = file + 1, r = rank + 1; f < 8 && r < 8; ++f, ++r) {
        int s = r * 8 + f;
        attacks |= 1ULL << s;
        if (occ & (1ULL << s)) break;
    }
    for (int f = file - 1, r = rank + 1; f >= 0 && r < 8; --f, ++r) {
        int s = r * 8 + f;
        attacks |= 1ULL << s;
        if (occ & (1ULL << s)) break;
    }
    for (int f = file + 1, r = rank - 1; f < 8 && r >= 0; ++f, --r) {
        int s = r * 8 + f;
        attacks |= 1ULL << s;
        if (occ & (1ULL << s)) break;
    }
    for (int f = file - 1, r = rank - 1; f >= 0 && r >= 0; --f, --r) {
        int s = r * 8 + f;
        attacks |= 1ULL << s;
        if (occ & (1ULL << s)) break;
    }
    return attacks;
}
