#include "perft.h"
#include "movegen.h"
#include "make.h"

uint64_t perft(Position *pos, int depth) {
    if (depth == 0) return 1;
    MoveList list;
    gen_pseudo_legal(pos, &list);
    uint64_t nodes = 0;
    for (int i = 0; i < list.n; ++i) {
        Move mv = list.m[i];
        if (!make_move(pos, mv)) continue;
        nodes += perft(pos, depth - 1);
        undo_move(pos, mv);
    }
    return nodes;
}
