#include <stdlib.h>
#include <string.h>
#include "tt.h"

void tt_init(TT *tt, size_t mb) {
    if (mb < 1) mb = 1;
    size_t bytes = mb * 1024u * 1024u;
    size_t n = bytes / sizeof(TTEntry);
    size_t pow2 = 1;
    while (pow2 < n) pow2 <<= 1;
    n = pow2;
    tt->t = (TTEntry *)calloc(n, sizeof(TTEntry));
    tt->n = n;
    tt->mask = (uint64_t)(n - 1);
}

void tt_free(TT *tt) {
    free(tt->t);
    tt->t = NULL;
    tt->n = 0;
    tt->mask = 0;
}

TTEntry *tt_probe(TT *tt, uint64_t key) {
    if (!tt->t) return NULL;
    return &tt->t[key & tt->mask];
}

void tt_store(TT *tt, uint64_t key, int depth, int score, TTFlag flag, Move best) {
    if (!tt->t) return;
    TTEntry *e = &tt->t[key & tt->mask];
    if (e->depth > depth && e->key16 == (uint16_t)key) return;
    e->key16 = (uint16_t)key;
    e->depth = (uint8_t)depth;
    e->flag = (uint8_t)flag;
    e->score = (int16_t)score;
    e->move16 = (uint16_t)(best & 0xFFFFu);
    e->move32 = best;
}
