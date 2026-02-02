#include "movegen.h"
#include "tables.h"
#include "attack.h"
#include "make.h"

static void add_move(MoveList *list, Move mv) {
    if (list->n < MAX_MOVES) list->m[list->n++] = mv;
}

void gen_pseudo_legal(const Position *pos, MoveList *list) {
    list->n = 0;
    int side = pos->side;
    U64 own = pos->bb_color[side];
    U64 opp = pos->bb_color[side ^ 1];
    U64 occ = pos->occ;

    if (side == WHITE) {
        U64 pawns = pos->bb_piece[WP - 1];
        while (pawns) {
            int from = lsb_index(pawns);
            pawns &= pawns - 1;
            int rank = from >> 3;
            int to = from + 8;
            if (to < 64 && !(occ & (1ULL << to))) {
                if (rank == 6) {
                    add_move(list, move_encode(from, to, WP, EMPTY, WQ, FLAG_PROMO));
                    add_move(list, move_encode(from, to, WP, EMPTY, WR, FLAG_PROMO));
                    add_move(list, move_encode(from, to, WP, EMPTY, WB, FLAG_PROMO));
                    add_move(list, move_encode(from, to, WP, EMPTY, WN, FLAG_PROMO));
                } else {
                    add_move(list, move_encode(from, to, WP, EMPTY, EMPTY, FLAG_NONE));
                    if (rank == 1) {
                        int to2 = from + 16;
                        if (!(occ & (1ULL << to2))) {
                            add_move(list, move_encode(from, to2, WP, EMPTY, EMPTY, FLAG_DBLPUSH));
                        }
                    }
                }
            }
            U64 caps = PAWN_ATTACKS[WHITE][from] & opp;
            while (caps) {
                int tosq = lsb_index(caps);
                caps &= caps - 1;
                Piece cap = pos->piece_on[tosq];
                if (rank == 6) {
                    add_move(list, move_encode(from, tosq, WP, cap, WQ, FLAG_CAPTURE | FLAG_PROMO));
                    add_move(list, move_encode(from, tosq, WP, cap, WR, FLAG_CAPTURE | FLAG_PROMO));
                    add_move(list, move_encode(from, tosq, WP, cap, WB, FLAG_CAPTURE | FLAG_PROMO));
                    add_move(list, move_encode(from, tosq, WP, cap, WN, FLAG_CAPTURE | FLAG_PROMO));
                } else {
                    add_move(list, move_encode(from, tosq, WP, cap, EMPTY, FLAG_CAPTURE));
                }
            }
            if (pos->ep_sq >= 0) {
                U64 ep_mask = 1ULL << pos->ep_sq;
                if (PAWN_ATTACKS[WHITE][from] & ep_mask) {
                    add_move(list, move_encode(from, pos->ep_sq, WP, BP, EMPTY, FLAG_EP | FLAG_CAPTURE));
                }
            }
        }
    } else {
        U64 pawns = pos->bb_piece[BP - 1];
        while (pawns) {
            int from = lsb_index(pawns);
            pawns &= pawns - 1;
            int rank = from >> 3;
            int to = from - 8;
            if (to >= 0 && !(occ & (1ULL << to))) {
                if (rank == 1) {
                    add_move(list, move_encode(from, to, BP, EMPTY, BQ, FLAG_PROMO));
                    add_move(list, move_encode(from, to, BP, EMPTY, BR, FLAG_PROMO));
                    add_move(list, move_encode(from, to, BP, EMPTY, BB, FLAG_PROMO));
                    add_move(list, move_encode(from, to, BP, EMPTY, BN, FLAG_PROMO));
                } else {
                    add_move(list, move_encode(from, to, BP, EMPTY, EMPTY, FLAG_NONE));
                    if (rank == 6) {
                        int to2 = from - 16;
                        if (!(occ & (1ULL << to2))) {
                            add_move(list, move_encode(from, to2, BP, EMPTY, EMPTY, FLAG_DBLPUSH));
                        }
                    }
                }
            }
            U64 caps = PAWN_ATTACKS[BLACK][from] & opp;
            while (caps) {
                int tosq = lsb_index(caps);
                caps &= caps - 1;
                Piece cap = pos->piece_on[tosq];
                if (rank == 1) {
                    add_move(list, move_encode(from, tosq, BP, cap, BQ, FLAG_CAPTURE | FLAG_PROMO));
                    add_move(list, move_encode(from, tosq, BP, cap, BR, FLAG_CAPTURE | FLAG_PROMO));
                    add_move(list, move_encode(from, tosq, BP, cap, BB, FLAG_CAPTURE | FLAG_PROMO));
                    add_move(list, move_encode(from, tosq, BP, cap, BN, FLAG_CAPTURE | FLAG_PROMO));
                } else {
                    add_move(list, move_encode(from, tosq, BP, cap, EMPTY, FLAG_CAPTURE));
                }
            }
            if (pos->ep_sq >= 0) {
                U64 ep_mask = 1ULL << pos->ep_sq;
                if (PAWN_ATTACKS[BLACK][from] & ep_mask) {
                    add_move(list, move_encode(from, pos->ep_sq, BP, WP, EMPTY, FLAG_EP | FLAG_CAPTURE));
                }
            }
        }
    }

    U64 knights = pos->bb_piece[(side == WHITE ? WN : BN) - 1];
    while (knights) {
        int from = lsb_index(knights);
        knights &= knights - 1;
        U64 targets = KNIGHT_ATTACKS[from] & ~own;
        while (targets) {
            int to = lsb_index(targets);
            targets &= targets - 1;
            Piece cap = pos->piece_on[to];
            uint32_t flags = cap != EMPTY ? FLAG_CAPTURE : FLAG_NONE;
            add_move(list, move_encode(from, to, pos->piece_on[from], cap, EMPTY, flags));
        }
    }

    U64 bishops = pos->bb_piece[(side == WHITE ? WB : BB) - 1];
    while (bishops) {
        int from = lsb_index(bishops);
        bishops &= bishops - 1;
        U64 targets = bishop_attacks(from, occ) & ~own;
        while (targets) {
            int to = lsb_index(targets);
            targets &= targets - 1;
            Piece cap = pos->piece_on[to];
            uint32_t flags = cap != EMPTY ? FLAG_CAPTURE : FLAG_NONE;
            add_move(list, move_encode(from, to, pos->piece_on[from], cap, EMPTY, flags));
        }
    }

    U64 rooks = pos->bb_piece[(side == WHITE ? WR : BR) - 1];
    while (rooks) {
        int from = lsb_index(rooks);
        rooks &= rooks - 1;
        U64 targets = rook_attacks(from, occ) & ~own;
        while (targets) {
            int to = lsb_index(targets);
            targets &= targets - 1;
            Piece cap = pos->piece_on[to];
            uint32_t flags = cap != EMPTY ? FLAG_CAPTURE : FLAG_NONE;
            add_move(list, move_encode(from, to, pos->piece_on[from], cap, EMPTY, flags));
        }
    }

    U64 queens = pos->bb_piece[(side == WHITE ? WQ : BQ) - 1];
    while (queens) {
        int from = lsb_index(queens);
        queens &= queens - 1;
        U64 targets = (rook_attacks(from, occ) | bishop_attacks(from, occ)) & ~own;
        while (targets) {
            int to = lsb_index(targets);
            targets &= targets - 1;
            Piece cap = pos->piece_on[to];
            uint32_t flags = cap != EMPTY ? FLAG_CAPTURE : FLAG_NONE;
            add_move(list, move_encode(from, to, pos->piece_on[from], cap, EMPTY, flags));
        }
    }

    int king_sq = pos->king_sq[side];
    U64 king_moves = KING_ATTACKS[king_sq] & ~own;
    while (king_moves) {
        int to = lsb_index(king_moves);
        king_moves &= king_moves - 1;
        Piece cap = pos->piece_on[to];
        uint32_t flags = cap != EMPTY ? FLAG_CAPTURE : FLAG_NONE;
        add_move(list, move_encode(king_sq, to, pos->piece_on[king_sq], cap, EMPTY, flags));
    }

    if (side == WHITE && king_sq == 4) {
        if ((pos->castle_rights & (1 << 0)) &&
            !(occ & ((1ULL << 5) | (1ULL << 6))) &&
            !is_square_attacked(pos, 4, BLACK) &&
            !is_square_attacked(pos, 5, BLACK) &&
            !is_square_attacked(pos, 6, BLACK)) {
            add_move(list, move_encode(4, 6, WK, EMPTY, EMPTY, FLAG_CASTLE));
        }
        if ((pos->castle_rights & (1 << 1)) &&
            !(occ & ((1ULL << 1) | (1ULL << 2) | (1ULL << 3))) &&
            !is_square_attacked(pos, 4, BLACK) &&
            !is_square_attacked(pos, 3, BLACK) &&
            !is_square_attacked(pos, 2, BLACK)) {
            add_move(list, move_encode(4, 2, WK, EMPTY, EMPTY, FLAG_CASTLE));
        }
    } else if (side == BLACK && king_sq == 60) {
        if ((pos->castle_rights & (1 << 2)) &&
            !(occ & ((1ULL << 61) | (1ULL << 62))) &&
            !is_square_attacked(pos, 60, WHITE) &&
            !is_square_attacked(pos, 61, WHITE) &&
            !is_square_attacked(pos, 62, WHITE)) {
            add_move(list, move_encode(60, 62, BK, EMPTY, EMPTY, FLAG_CASTLE));
        }
        if ((pos->castle_rights & (1 << 3)) &&
            !(occ & ((1ULL << 57) | (1ULL << 58) | (1ULL << 59))) &&
            !is_square_attacked(pos, 60, WHITE) &&
            !is_square_attacked(pos, 59, WHITE) &&
            !is_square_attacked(pos, 58, WHITE)) {
            add_move(list, move_encode(60, 58, BK, EMPTY, EMPTY, FLAG_CASTLE));
        }
    }
}

bool is_legal_move(Position *pos, Move mv) {
    if (!make_move(pos, mv)) return false;
    undo_move(pos, mv);
    return true;
}
