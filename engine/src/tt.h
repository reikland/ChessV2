#pragma once
#include "types.h"
#include "move.h"

typedef enum { TT_EMPTY = 0, TT_EXACT = 1, TT_LOWER = 2, TT_UPPER = 3 } TTFlag;

typedef struct {
    uint16_t key16;
    uint8_t depth;
    uint8_t flag;
    int16_t score;
    uint16_t move16;
    uint16_t pad;
    uint32_t move32;
} TTEntry;

typedef struct {
    TTEntry *t;
    size_t n;
    uint64_t mask;
} TT;

void tt_init(TT *tt, size_t mb);
void tt_free(TT *tt);
TTEntry *tt_probe(TT *tt, uint64_t key);
void tt_store(TT *tt, uint64_t key, int depth, int score, TTFlag flag, Move best);
