#include <string.h>
#include <limits.h>
#include "search.h"
#include "movegen.h"
#include "make.h"
#include "eval.h"
#include "time.h"
#include "tables.h"

#define INF 32000
#define MATE 30000

static SearchLimits CURRENT_LIMITS;

static bool time_up(void) {
    if (CURRENT_LIMITS.stop) return true;
    uint64_t now = now_ms();
    if (CURRENT_LIMITS.hard_stop_ms && now >= CURRENT_LIMITS.hard_stop_ms) {
        return true;
    }
    return false;
}

static int mvv_lva(Piece attacker, Piece victim) {
    static const int val[13] = {0, 1, 3, 3, 5, 9, 10, 1, 3, 3, 5, 9, 10};
    return val[victim] * 16 - val[attacker];
}

static int score_move(const SearchCtx *ctx, Move mv, Move tt_move, int ply) {
    if (mv == tt_move) return 1000000;
    Piece cap = M_CAP(mv);
    if (M_FLAGS(mv) & FLAG_CAPTURE) {
        return 900000 + mvv_lva(M_PIECE(mv), cap == EMPTY ? WP : cap);
    }
    if (ctx->killer[ply][0] == (int)mv) return 800000;
    if (ctx->killer[ply][1] == (int)mv) return 799000;
    return ctx->history[M_PIECE(mv) - 1][M_TO(mv)];
}

static void sort_moves(SearchCtx *ctx, MoveList *list, Move tt_move, int ply) {
    for (int i = 0; i < list->n; ++i) {
        for (int j = i + 1; j < list->n; ++j) {
            int si = score_move(ctx, list->m[i], tt_move, ply);
            int sj = score_move(ctx, list->m[j], tt_move, ply);
            if (sj > si) {
                Move tmp = list->m[i];
                list->m[i] = list->m[j];
                list->m[j] = tmp;
            }
        }
    }
}

static int qsearch(SearchCtx *ctx, Position *pos, int alpha, int beta, int ply) {
    if (time_up()) return eval(pos);
    CURRENT_LIMITS.nodes++;

    int stand_pat = eval(pos);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    MoveList list;
    gen_pseudo_legal(pos, &list);
    for (int i = 0; i < list.n; ++i) {
        Move mv = list.m[i];
        if (!(M_FLAGS(mv) & FLAG_CAPTURE)) continue;
        if (!make_move(pos, mv)) continue;
        int score = -qsearch(ctx, pos, -beta, -alpha, ply + 1);
        undo_move(pos, mv);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

static int negamax(SearchCtx *ctx, Position *pos, int depth, int alpha, int beta, int ply) {
    if (time_up()) return eval(pos);
    if (depth <= 0) return qsearch(ctx, pos, alpha, beta, ply);

    CURRENT_LIMITS.nodes++;

    int alpha_orig = alpha;

    TTEntry *entry = tt_probe(&ctx->tt, pos->key);
    Move tt_move = 0;
    if (entry && entry->key16 == (uint16_t)pos->key) {
        tt_move = entry->move32;
        if (entry->depth >= depth) {
            int tt_score = entry->score;
            if (entry->flag == TT_EXACT) return tt_score;
            if (entry->flag == TT_LOWER && tt_score > alpha) alpha = tt_score;
            else if (entry->flag == TT_UPPER && tt_score < beta) beta = tt_score;
            if (alpha >= beta) return tt_score;
        }
    }

    MoveList list;
    gen_pseudo_legal(pos, &list);
    if (list.n == 0) {
        if (in_check(pos, pos->side)) return -MATE + ply;
        return 0;
    }

    sort_moves(ctx, &list, tt_move, ply);

    int best_score = -INF;
    Move best_move = 0;
    int legal_moves = 0;

    for (int i = 0; i < list.n; ++i) {
        Move mv = list.m[i];
        if (!make_move(pos, mv)) continue;
        legal_moves++;
        int score = -negamax(ctx, pos, depth - 1, -beta, -alpha, ply + 1);
        undo_move(pos, mv);

        if (score > best_score) {
            best_score = score;
            best_move = mv;
        }
        if (score > alpha) {
            alpha = score;
            if (!(M_FLAGS(mv) & FLAG_CAPTURE)) {
                ctx->history[M_PIECE(mv) - 1][M_TO(mv)] += depth * depth;
            }
        }
        if (alpha >= beta) {
            if (!(M_FLAGS(mv) & FLAG_CAPTURE)) {
                ctx->killer[ply][1] = ctx->killer[ply][0];
                ctx->killer[ply][0] = (int)mv;
            }
            break;
        }
    }

    if (legal_moves == 0) {
        if (in_check(pos, pos->side)) return -MATE + ply;
        return 0;
    }

    TTFlag flag = TT_EXACT;
    if (best_score <= alpha_orig) flag = TT_UPPER;
    else if (best_score >= beta) flag = TT_LOWER;
    tt_store(&ctx->tt, pos->key, depth, best_score, flag, best_move);

    return best_score;
}

void search_init(SearchCtx *ctx, size_t tt_mb) {
    memset(ctx, 0, sizeof(*ctx));
    tt_init(&ctx->tt, tt_mb);
    tables_init();
}

void search_quit(SearchCtx *ctx) {
    tt_free(&ctx->tt);
}

Move search_bestmove(SearchCtx *ctx, Position *pos, const SearchLimits *lim) {
    CURRENT_LIMITS = *lim;
    CURRENT_LIMITS.nodes = 0;
    CURRENT_LIMITS.start_ms = now_ms();

    uint64_t time_budget = 0;
    if (lim->movetime_ms > 0) {
        time_budget = (uint64_t)lim->movetime_ms;
    } else if (lim->wtime_ms > 0 || lim->btime_ms > 0) {
        int side_time = pos->side == WHITE ? lim->wtime_ms : lim->btime_ms;
        int inc = pos->side == WHITE ? lim->winc_ms : lim->binc_ms;
        time_budget = (uint64_t)(side_time / 25 + inc);
    } else {
        time_budget = 1000;
    }

    CURRENT_LIMITS.soft_stop_ms = CURRENT_LIMITS.start_ms + time_budget;
    CURRENT_LIMITS.hard_stop_ms = CURRENT_LIMITS.start_ms + time_budget + 50;

    Move best = 0;
    int best_score = -INF;
    int max_depth = lim->max_depth > 0 ? lim->max_depth : 5;

    int window = 30;
    int alpha = -INF;
    int beta = INF;

    for (int depth = 1; depth <= max_depth; ++depth) {
        int score = negamax(ctx, pos, depth, alpha, beta, 0);
        if (time_up()) break;
        if (score <= alpha || score >= beta) {
            alpha = -INF;
            beta = INF;
            score = negamax(ctx, pos, depth, alpha, beta, 0);
        }
        best_score = score;
        TTEntry *e = tt_probe(&ctx->tt, pos->key);
        if (e && e->key16 == (uint16_t)pos->key) {
            best = e->move32;
        }
        alpha = best_score - window;
        beta = best_score + window;
    }

    return best;
}
