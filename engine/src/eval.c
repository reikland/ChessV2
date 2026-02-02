#include "eval.h"
#include "tables.h"

static const int PIECE_VALUE[13] = {
    0, 100, 320, 330, 500, 900, 20000,
    100, 320, 330, 500, 900, 20000
};

static const int PST_PAWN[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     50,  50,  50,  50,  50,  50,  50,  50,
     10,  10,  20,  30,  30,  20,  10,  10,
      5,   5,  10,  25,  25,  10,   5,   5,
      0,   0,   0,  20,  20,   0,   0,   0,
      5,  -5, -10,   0,   0, -10,  -5,   5,
      5,  10,  10, -20, -20,  10,  10,   5,
      0,   0,   0,   0,   0,   0,   0,   0
};

static const int PST_KNIGHT[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

static const int PST_BISHOP[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

static const int PST_ROOK[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
     5,  10,  10,  10,  10,  10,  10,   5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
     0,   0,   0,   5,   5,   0,   0,   0
};

static const int PST_QUEEN[64] = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
      0,   0,   5,   5,   5,   5,   0,  -5,
    -10,   5,   5,   5,   5,   5,   0, -10,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

static const int PST_KING[64] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     20,  20,   0,   0,   0,   0,  20,  20,
     20,  30,  10,   0,   0,  10,  30,  20
};

static int mirror_sq(int sq) {
    int file = sq & 7;
    int rank = sq >> 3;
    return (7 - rank) * 8 + file;
}

static int eval_pst(Piece p, int sq) {
    switch (p) {
        case WP: return PST_PAWN[sq];
        case BP: return PST_PAWN[mirror_sq(sq)];
        case WN: return PST_KNIGHT[sq];
        case BN: return PST_KNIGHT[mirror_sq(sq)];
        case WB: return PST_BISHOP[sq];
        case BB: return PST_BISHOP[mirror_sq(sq)];
        case WR: return PST_ROOK[sq];
        case BR: return PST_ROOK[mirror_sq(sq)];
        case WQ: return PST_QUEEN[sq];
        case BQ: return PST_QUEEN[mirror_sq(sq)];
        case WK: return PST_KING[sq];
        case BK: return PST_KING[mirror_sq(sq)];
        default: return 0;
    }
}

int eval(const Position *pos) {
    int score = 0;
    int white_bishops = 0;
    int black_bishops = 0;

    int white_pawns_file[8] = {0};
    int black_pawns_file[8] = {0};

    for (int sq = 0; sq < 64; ++sq) {
        Piece p = pos->piece_on[sq];
        if (p == EMPTY) continue;
        int val = PIECE_VALUE[p];
        int pst = eval_pst(p, sq);
        if (piece_color(p) == WHITE) {
            score += val + pst;
        } else {
            score -= val + pst;
        }
        if (p == WB) white_bishops++;
        if (p == BB) black_bishops++;
        if (p == WP) white_pawns_file[sq & 7]++;
        if (p == BP) black_pawns_file[sq & 7]++;
    }

    if (white_bishops >= 2) score += 30;
    if (black_bishops >= 2) score -= 30;

    for (int file = 0; file < 8; ++file) {
        if (white_pawns_file[file] > 1) score -= 15 * (white_pawns_file[file] - 1);
        if (black_pawns_file[file] > 1) score += 15 * (black_pawns_file[file] - 1);
    }

    for (int file = 0; file < 8; ++file) {
        if (white_pawns_file[file] > 0) {
            int adj = 0;
            if (file > 0) adj += white_pawns_file[file - 1];
            if (file < 7) adj += white_pawns_file[file + 1];
            if (adj == 0) score -= 10;
        }
        if (black_pawns_file[file] > 0) {
            int adj = 0;
            if (file > 0) adj += black_pawns_file[file - 1];
            if (file < 7) adj += black_pawns_file[file + 1];
            if (adj == 0) score += 10;
        }
    }

    int mobility_white = popcount64(KNIGHT_ATTACKS[pos->king_sq[WHITE]]);
    int mobility_black = popcount64(KNIGHT_ATTACKS[pos->king_sq[BLACK]]);
    score += (mobility_white - mobility_black);

    return (pos->side == WHITE) ? score : -score;
}
