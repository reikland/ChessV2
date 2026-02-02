#include "attack.h"
#include "tables.h"

bool is_square_attacked(const Position *pos, int sq, int by_side) {
    U64 occ = pos->occ;
    if (by_side == WHITE) {
        if (PAWN_ATTACKS[BLACK][sq] & pos->bb_piece[WP - 1]) return true;
        if (KNIGHT_ATTACKS[sq] & pos->bb_piece[WN - 1]) return true;
        if (KING_ATTACKS[sq] & pos->bb_piece[WK - 1]) return true;
        U64 bishops = pos->bb_piece[WB - 1] | pos->bb_piece[WQ - 1];
        if (bishop_attacks(sq, occ) & bishops) return true;
        U64 rooks = pos->bb_piece[WR - 1] | pos->bb_piece[WQ - 1];
        if (rook_attacks(sq, occ) & rooks) return true;
    } else {
        if (PAWN_ATTACKS[WHITE][sq] & pos->bb_piece[BP - 1]) return true;
        if (KNIGHT_ATTACKS[sq] & pos->bb_piece[BN - 1]) return true;
        if (KING_ATTACKS[sq] & pos->bb_piece[BK - 1]) return true;
        U64 bishops = pos->bb_piece[BB - 1] | pos->bb_piece[BQ - 1];
        if (bishop_attacks(sq, occ) & bishops) return true;
        U64 rooks = pos->bb_piece[BR - 1] | pos->bb_piece[BQ - 1];
        if (rook_attacks(sq, occ) & rooks) return true;
    }
    return false;
}
