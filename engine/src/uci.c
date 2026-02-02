#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "uci.h"
#include "position.h"
#include "search.h"
#include "zobrist.h"
#include "movegen.h"
#include "make.h"
#include "perft.h"

static Position POS;
static SearchCtx CTX;

static const char *STARTPOS_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static void set_startpos(Position *p) {
    pos_from_fen(p, STARTPOS_FEN);
}

static void move_to_uci(Move mv, char *out) {
    int from = M_FROM(mv);
    int to = M_TO(mv);
    out[0] = (char)('a' + (from & 7));
    out[1] = (char)('1' + (from >> 3));
    out[2] = (char)('a' + (to & 7));
    out[3] = (char)('1' + (to >> 3));
    int idx = 4;
    if (M_FLAGS(mv) & FLAG_PROMO) {
        Piece p = M_PROMO(mv);
        char c = 'q';
        if (p == WR || p == BR) c = 'r';
        else if (p == WB || p == BB) c = 'b';
        else if (p == WN || p == BN) c = 'n';
        out[idx++] = c;
    }
    out[idx] = '\0';
}

static Move uci_move_from_str(Position *pos, const char *str) {
    MoveList list;
    gen_pseudo_legal(pos, &list);
    for (int i = 0; i < list.n; ++i) {
        Move mv = list.m[i];
        if (!is_legal_move(pos, mv)) continue;
        char buf[6];
        move_to_uci(mv, buf);
        if (strlen(buf) == strlen(str) && !strcmp(buf, str)) return mv;
    }
    return 0;
}

static void parse_position(Position *pos, char *line) {
    char *token = strtok(line, " \n");
    token = strtok(NULL, " \n");
    if (!token) return;

    if (!strcmp(token, "startpos")) {
        set_startpos(pos);
        token = strtok(NULL, " \n");
    } else if (!strcmp(token, "fen")) {
        char fen[256] = {0};
        char *ptr = fen;
        int parts = 0;
        while ((token = strtok(NULL, " \n")) != NULL) {
            if (!strcmp(token, "moves")) break;
            if (parts > 0) *ptr++ = ' ';
            size_t len = strlen(token);
            memcpy(ptr, token, len);
            ptr += len;
            parts++;
            if (parts >= 6) {
                token = strtok(NULL, " \n");
                break;
            }
        }
        pos_from_fen(pos, fen);
    }

    if (token && !strcmp(token, "moves")) {
        while ((token = strtok(NULL, " \n")) != NULL) {
            Move mv = uci_move_from_str(pos, token);
            if (mv) make_move(pos, mv);
        }
    }
}

static void parse_go(SearchLimits *lim, char *line) {
    memset(lim, 0, sizeof(*lim));
    char *token = strtok(line, " \n");
    while ((token = strtok(NULL, " \n")) != NULL) {
        if (!strcmp(token, "depth")) {
            token = strtok(NULL, " \n");
            lim->max_depth = token ? atoi(token) : 0;
        } else if (!strcmp(token, "movetime")) {
            token = strtok(NULL, " \n");
            lim->movetime_ms = token ? atoi(token) : 0;
        } else if (!strcmp(token, "wtime")) {
            token = strtok(NULL, " \n");
            lim->wtime_ms = token ? atoi(token) : 0;
        } else if (!strcmp(token, "btime")) {
            token = strtok(NULL, " \n");
            lim->btime_ms = token ? atoi(token) : 0;
        } else if (!strcmp(token, "winc")) {
            token = strtok(NULL, " \n");
            lim->winc_ms = token ? atoi(token) : 0;
        } else if (!strcmp(token, "binc")) {
            token = strtok(NULL, " \n");
            lim->binc_ms = token ? atoi(token) : 0;
        }
    }
}

void uci_loop(void) {
    char line[4096];
    zobrist_init(20260202ULL);
    search_init(&CTX, 128);
    set_startpos(&POS);

    while (fgets(line, sizeof(line), stdin)) {
        if (!strncmp(line, "uci", 3)) {
            printf("id name CEngine\n");
            printf("id author you\n");
            printf("uciok\n");
            fflush(stdout);
        } else if (!strncmp(line, "isready", 7)) {
            printf("readyok\n");
            fflush(stdout);
        } else if (!strncmp(line, "ucinewgame", 10)) {
            set_startpos(&POS);
        } else if (!strncmp(line, "position", 8)) {
            parse_position(&POS, line);
        } else if (!strncmp(line, "go", 2)) {
            SearchLimits lim;
            parse_go(&lim, line);
            Move best = search_bestmove(&CTX, &POS, &lim);
            char buf[8];
            if (best) move_to_uci(best, buf);
            else strcpy(buf, "0000");
            printf("bestmove %s\n", buf);
            fflush(stdout);
        } else if (!strncmp(line, "perft", 5)) {
            int depth = atoi(line + 6);
            uint64_t nodes = perft(&POS, depth);
            printf("perft %d nodes %llu\n", depth, (unsigned long long)nodes);
            fflush(stdout);
        } else if (!strncmp(line, "quit", 4)) {
            break;
        }
    }

    search_quit(&CTX);
}
