#include "make.h"
#include "attack.h"
#include "zobrist.h"

static inline void remove_piece(Position *pos, Piece p, int sq) {
    pos->bb_piece[p - 1] &= ~(1ULL << sq);
    pos->piece_on[sq] = EMPTY;
    pos->key ^= Z_PIECE[p - 1][sq];
}

static inline void add_piece(Position *pos, Piece p, int sq) {
    pos->bb_piece[p - 1] |= 1ULL << sq;
    pos->piece_on[sq] = p;
    pos->key ^= Z_PIECE[p - 1][sq];
}

bool in_check(const Position *pos, int side) {
    int ksq = pos->king_sq[side];
    return is_square_attacked(pos, ksq, side ^ 1);
}

static void update_castle(Position *pos, Piece pc, int from, int to) {
    uint8_t rights = pos->castle_rights;
    if (pc == WK) rights &= (uint8_t)~((1u << 0) | (1u << 1));
    if (pc == BK) rights &= (uint8_t)~((1u << 2) | (1u << 3));
    if (pc == WR) {
        if (from == 0) rights &= (uint8_t)~(1u << 1);
        if (from == 7) rights &= (uint8_t)~(1u << 0);
    }
    if (pc == BR) {
        if (from == 56) rights &= (uint8_t)~(1u << 3);
        if (from == 63) rights &= (uint8_t)~(1u << 2);
    }
    if (to == 0) rights &= (uint8_t)~(1u << 1);
    if (to == 7) rights &= (uint8_t)~(1u << 0);
    if (to == 56) rights &= (uint8_t)~(1u << 3);
    if (to == 63) rights &= (uint8_t)~(1u << 2);
    if (rights != pos->castle_rights) {
        pos->key ^= Z_CASTLE[pos->castle_rights & 15u];
        pos->castle_rights = rights;
        pos->key ^= Z_CASTLE[pos->castle_rights & 15u];
    }
}

bool make_move(Position *pos, Move mv) {
    State *st = &pos->st[pos->ply];
    st->key = pos->key;
    st->ep_sq = pos->ep_sq;
    st->castle_rights = pos->castle_rights;
    st->halfmove_clock = pos->halfmove_clock;
    st->captured = EMPTY;

    int from = M_FROM(mv);
    int to = M_TO(mv);
    Piece pc = M_PIECE(mv);
    Piece cap = M_CAP(mv);
    uint32_t flags = M_FLAGS(mv);

    if (pos->ep_sq >= 0) {
        pos->key ^= Z_EPFILE[pos->ep_sq & 7];
    } else {
        pos->key ^= Z_EPFILE[8];
    }
    pos->ep_sq = -1;
    pos->key ^= Z_EPFILE[8];

    update_castle(pos, pc, from, to);

    remove_piece(pos, pc, from);

    if (flags & FLAG_EP) {
        int cap_sq = to + (pos->side == WHITE ? -8 : 8);
        Piece cap_piece = pos->side == WHITE ? BP : WP;
        remove_piece(pos, cap_piece, cap_sq);
        st->captured = cap_piece;
    } else if (flags & FLAG_CAPTURE) {
        if (cap == EMPTY) cap = pos->piece_on[to];
        if (cap != EMPTY) {
            remove_piece(pos, cap, to);
            st->captured = cap;
        }
    }

    if (flags & FLAG_PROMO) {
        Piece promo = M_PROMO(mv);
        add_piece(pos, promo, to);
    } else {
        add_piece(pos, pc, to);
    }

    if (pc == WK) pos->king_sq[WHITE] = to;
    if (pc == BK) pos->king_sq[BLACK] = to;

    if (flags & FLAG_CASTLE) {
        if (to == 6) {
            remove_piece(pos, WR, 7);
            add_piece(pos, WR, 5);
        } else if (to == 2) {
            remove_piece(pos, WR, 0);
            add_piece(pos, WR, 3);
        } else if (to == 62) {
            remove_piece(pos, BR, 63);
            add_piece(pos, BR, 61);
        } else if (to == 58) {
            remove_piece(pos, BR, 56);
            add_piece(pos, BR, 59);
        }
    }

    if (flags & FLAG_DBLPUSH) {
        pos->ep_sq = to + (pos->side == WHITE ? -8 : 8);
        pos->key ^= Z_EPFILE[8];
        pos->key ^= Z_EPFILE[pos->ep_sq & 7];
    }

    if (pc == WP || pc == BP || (flags & FLAG_CAPTURE)) {
        pos->halfmove_clock = 0;
    } else {
        pos->halfmove_clock++;
    }

    pos->side ^= 1;
    pos->key ^= Z_SIDE;
    pos->ply++;

    pos_update_occupancy(pos);

    if (in_check(pos, pos->side ^ 1)) {
        undo_move(pos, mv);
        return false;
    }
    return true;
}

void undo_move(Position *pos, Move mv) {
    pos->ply--;
    State *st = &pos->st[pos->ply];

    int from = M_FROM(mv);
    int to = M_TO(mv);
    Piece pc = M_PIECE(mv);
    uint32_t flags = M_FLAGS(mv);

    pos->side ^= 1;

    if (flags & FLAG_CASTLE) {
        if (to == 6) {
            remove_piece(pos, WR, 5);
            add_piece(pos, WR, 7);
        } else if (to == 2) {
            remove_piece(pos, WR, 3);
            add_piece(pos, WR, 0);
        } else if (to == 62) {
            remove_piece(pos, BR, 61);
            add_piece(pos, BR, 63);
        } else if (to == 58) {
            remove_piece(pos, BR, 59);
            add_piece(pos, BR, 56);
        }
    }

    if (flags & FLAG_PROMO) {
        Piece promo = M_PROMO(mv);
        remove_piece(pos, promo, to);
        add_piece(pos, pc, from);
    } else {
        remove_piece(pos, pc, to);
        add_piece(pos, pc, from);
    }

    if (pc == WK) pos->king_sq[WHITE] = from;
    if (pc == BK) pos->king_sq[BLACK] = from;

    if (flags & FLAG_EP) {
        int cap_sq = to + (pos->side == WHITE ? -8 : 8);
        Piece cap_piece = pos->side == WHITE ? BP : WP;
        add_piece(pos, cap_piece, cap_sq);
    } else if (flags & FLAG_CAPTURE) {
        if (st->captured != EMPTY) add_piece(pos, st->captured, to);
    }

    pos->key = st->key;
    pos->ep_sq = st->ep_sq;
    pos->castle_rights = st->castle_rights;
    pos->halfmove_clock = st->halfmove_clock;

    pos_update_occupancy(pos);
}
