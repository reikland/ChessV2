#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "position.h"
#include "zobrist.h"

static void pos_clear(Position *pos) {
    memset(pos, 0, sizeof(*pos));
    for (int i = 0; i < 64; ++i) pos->piece_on[i] = EMPTY;
    pos->ep_sq = -1;
    pos->castle_rights = 0;
    pos->halfmove_clock = 0;
    pos->side = WHITE;
    pos->king_sq[WHITE] = -1;
    pos->king_sq[BLACK] = -1;
}

void pos_update_occupancy(Position *pos) {
    pos->bb_color[WHITE] = 0;
    pos->bb_color[BLACK] = 0;
    for (int p = WP; p <= WK; ++p) pos->bb_color[WHITE] |= pos->bb_piece[p - 1];
    for (int p = BP; p <= BK; ++p) pos->bb_color[BLACK] |= pos->bb_piece[p - 1];
    pos->occ = pos->bb_color[WHITE] | pos->bb_color[BLACK];
}

void pos_compute_key(Position *pos) {
    uint64_t key = 0;
    for (int sq = 0; sq < 64; ++sq) {
        Piece p = pos->piece_on[sq];
        if (p != EMPTY) key ^= Z_PIECE[p - 1][sq];
    }
    key ^= Z_CASTLE[pos->castle_rights & 15u];
    if (pos->ep_sq >= 0) {
        int file = pos->ep_sq & 7;
        key ^= Z_EPFILE[file];
    } else {
        key ^= Z_EPFILE[8];
    }
    if (pos->side == BLACK) key ^= Z_SIDE;
    pos->key = key;
}

static int parse_piece(char c) {
    switch (c) {
        case 'P': return WP;
        case 'N': return WN;
        case 'B': return WB;
        case 'R': return WR;
        case 'Q': return WQ;
        case 'K': return WK;
        case 'p': return BP;
        case 'n': return BN;
        case 'b': return BB;
        case 'r': return BR;
        case 'q': return BQ;
        case 'k': return BK;
        default: return EMPTY;
    }
}

bool pos_from_fen(Position *pos, const char *fen) {
    if (!fen) return false;
    pos_clear(pos);

    int sq = 56;
    const char *p = fen;
    while (*p && *p != ' ') {
        if (*p == '/') {
            sq -= 16;
            ++p;
            continue;
        }
        if (isdigit((unsigned char)*p)) {
            int empty = *p - '0';
            for (int i = 0; i < empty; ++i) {
                pos->piece_on[sq++] = EMPTY;
            }
            ++p;
            continue;
        }
        int piece = parse_piece(*p);
        if (piece == EMPTY) return false;
        pos->piece_on[sq] = (Piece)piece;
        pos->bb_piece[piece - 1] |= 1ULL << sq;
        if (piece == WK) pos->king_sq[WHITE] = sq;
        if (piece == BK) pos->king_sq[BLACK] = sq;
        ++sq;
        ++p;
    }

    if (*p != ' ') return false;
    ++p;
    if (*p == 'w') pos->side = WHITE;
    else if (*p == 'b') pos->side = BLACK;
    else return false;
    while (*p && *p != ' ') ++p;
    if (*p != ' ') return false;
    ++p;

    pos->castle_rights = 0;
    if (*p == '-') {
        ++p;
    } else {
        while (*p && *p != ' ') {
            switch (*p) {
                case 'K': pos->castle_rights |= 1 << 0; break;
                case 'Q': pos->castle_rights |= 1 << 1; break;
                case 'k': pos->castle_rights |= 1 << 2; break;
                case 'q': pos->castle_rights |= 1 << 3; break;
                default: return false;
            }
            ++p;
        }
    }
    if (*p != ' ') return false;
    ++p;

    if (*p == '-') {
        pos->ep_sq = -1;
        ++p;
    } else {
        if (p[0] < 'a' || p[0] > 'h' || p[1] < '1' || p[1] > '8') return false;
        int file = p[0] - 'a';
        int rank = p[1] - '1';
        pos->ep_sq = rank * 8 + file;
        p += 2;
    }
    if (*p != ' ') return false;
    ++p;

    pos->halfmove_clock = (uint8_t)atoi(p);
    while (*p && *p != ' ') ++p;
    if (*p == ' ') {
        ++p;
        pos->ply = atoi(p);
    } else {
        pos->ply = 0;
    }

    pos_update_occupancy(pos);
    pos_compute_key(pos);
    return true;
}
