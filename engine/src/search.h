#pragma once
#include "position.h"
#include "tt.h"

typedef struct {
    int max_depth;
    int movetime_ms;
    int wtime_ms, btime_ms, winc_ms, binc_ms;
    int stop;
    uint64_t nodes;
    uint64_t start_ms;
    uint64_t hard_stop_ms;
    uint64_t soft_stop_ms;
} SearchLimits;

typedef struct {
    TT tt;
    int killer[MAX_PLY][2];
    int history[12][64];
} SearchCtx;

void search_init(SearchCtx *ctx, size_t tt_mb);
void search_quit(SearchCtx *ctx);

Move search_bestmove(SearchCtx *ctx, Position *pos, const SearchLimits *lim);
