#include "zobrist.h"

uint64_t Z_PIECE[12][64];
uint64_t Z_CASTLE[16];
uint64_t Z_EPFILE[9];
uint64_t Z_SIDE;

static uint64_t splitmix64(uint64_t *x) {
    uint64_t z = (*x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void zobrist_init(uint64_t seed) {
    uint64_t x = seed ? seed : 0x123456789abcdefULL;
    for (int p = 0; p < 12; ++p) {
        for (int sq = 0; sq < 64; ++sq) {
            Z_PIECE[p][sq] = splitmix64(&x);
        }
    }
    for (int i = 0; i < 16; ++i) {
        Z_CASTLE[i] = splitmix64(&x);
    }
    for (int i = 0; i < 9; ++i) {
        Z_EPFILE[i] = splitmix64(&x);
    }
    Z_SIDE = splitmix64(&x);
}
