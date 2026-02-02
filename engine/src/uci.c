#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "uci.h"
#include "position.h"
#include "search.h"
#include "zobrist.h"
#include "movegen.h"
#include "make.h"
#include "perft.h"

static Position POS;
static SearchCtx CTX;
static int LAST_FROM = -1;
static int LAST_TO = -1;
static Move MOVE_HISTORY[2048];
static int MOVE_COUNT = 0;
static int HUMAN_SIDE[2] = {1, 1};
static int VS_MODE = 0;
static int COMPUTER_MOVETIME_MS = 500;
static int START_DELAY_MS = 0;
static int PENDING_START_DELAY = 0;

static const char *STARTPOS_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static void set_startpos(Position *p) {
    pos_from_fen(p, STARTPOS_FEN);
    LAST_FROM = -1;
    LAST_TO = -1;
    MOVE_COUNT = 0;
    PENDING_START_DELAY = 1;
}

static void sleep_ms(int ms) {
    if (ms <= 0) return;
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
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

static void record_move(Move mv) {
    if (MOVE_COUNT < (int)(sizeof(MOVE_HISTORY) / sizeof(MOVE_HISTORY[0]))) {
        MOVE_HISTORY[MOVE_COUNT++] = mv;
    }
}

static void update_last_move_from_history(void) {
    if (MOVE_COUNT > 0) {
        Move mv = MOVE_HISTORY[MOVE_COUNT - 1];
        LAST_FROM = M_FROM(mv);
        LAST_TO = M_TO(mv);
    } else {
        LAST_FROM = -1;
        LAST_TO = -1;
    }
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
        MOVE_COUNT = 0;
        PENDING_START_DELAY = 1;
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
        LAST_FROM = -1;
        LAST_TO = -1;
    }

    if (token && !strcmp(token, "moves")) {
        while ((token = strtok(NULL, " \n")) != NULL) {
            Move mv = uci_move_from_str(pos, token);
            if (mv && make_move(pos, mv)) {
                LAST_FROM = M_FROM(mv);
                LAST_TO = M_TO(mv);
                record_move(mv);
            }
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

static void maybe_play_computer(Position *pos) {
    if (!VS_MODE) return;
    if (HUMAN_SIDE[pos->side]) return;
    if (PENDING_START_DELAY) {
        sleep_ms(START_DELAY_MS);
        PENDING_START_DELAY = 0;
    }
    SearchLimits lim;
    memset(&lim, 0, sizeof(lim));
    lim.movetime_ms = COMPUTER_MOVETIME_MS;
    int moved_side = pos->side;
    Move best = search_bestmove(&CTX, pos, &lim);
    if (best && make_move(pos, best)) {
        record_move(best);
        LAST_FROM = M_FROM(best);
        LAST_TO = M_TO(best);
        printf("computer %s\n", moved_side == WHITE ? "white" : "black");
        pos_print_pretty(pos, LAST_FROM, LAST_TO);
        fflush(stdout);
    } else {
        printf("computer has no legal moves\n");
        fflush(stdout);
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
            pos_print_pretty(&POS, LAST_FROM, LAST_TO);
            fflush(stdout);
        } else if (!strncmp(line, "mode", 4)) {
            char *token = strtok(line, " \n");
            char *color = strtok(NULL, " \n");
            char *role = strtok(NULL, " \n");
            if (color && role) {
                VS_MODE = 1;
                if (!strcmp(color, "white")) {
                    HUMAN_SIDE[WHITE] = !strcmp(role, "human");
                    HUMAN_SIDE[BLACK] = !HUMAN_SIDE[WHITE];
                } else if (!strcmp(color, "black")) {
                    HUMAN_SIDE[BLACK] = !strcmp(role, "human");
                    HUMAN_SIDE[WHITE] = !HUMAN_SIDE[BLACK];
                }
                printf("mode white %s vs black %s\n",
                       HUMAN_SIDE[WHITE] ? "human" : "computer",
                       HUMAN_SIDE[BLACK] ? "human" : "computer");
                fflush(stdout);
                maybe_play_computer(&POS);
            } else {
                printf("usage: mode <white|black> <human|computer>\n");
                fflush(stdout);
            }
        } else if (!strncmp(line, "thinktime", 9)) {
            int ms = atoi(line + 9);
            if (ms > 0) COMPUTER_MOVETIME_MS = ms;
            printf("thinktime %d\n", COMPUTER_MOVETIME_MS);
            fflush(stdout);
        } else if (!strncmp(line, "startdelay", 10)) {
            int ms = atoi(line + 10);
            if (ms >= 0) START_DELAY_MS = ms;
            printf("startdelay %d\n", START_DELAY_MS);
            fflush(stdout);
        } else if (!strncmp(line, "move", 4)) {
            char *token = strtok(line, " \n");
            char *uci = strtok(NULL, " \n");
            if (!uci) {
                printf("usage: move <uci>\n");
                fflush(stdout);
            } else {
                Move mv = uci_move_from_str(&POS, uci);
                if (mv && make_move(&POS, mv)) {
                    record_move(mv);
                    LAST_FROM = M_FROM(mv);
                    LAST_TO = M_TO(mv);
                    pos_print_pretty(&POS, LAST_FROM, LAST_TO);
                    fflush(stdout);
                    maybe_play_computer(&POS);
                } else {
                    printf("illegal move %s\n", uci);
                    fflush(stdout);
                }
            }
        } else if (!strncmp(line, "undo", 4)) {
            int count = 1;
            if (strlen(line) > 4) {
                count = atoi(line + 4);
                if (count <= 0) count = 1;
            }
            while (count-- > 0 && MOVE_COUNT > 0) {
                Move mv = MOVE_HISTORY[--MOVE_COUNT];
                undo_move(&POS, mv);
            }
            update_last_move_from_history();
            pos_print_pretty(&POS, LAST_FROM, LAST_TO);
            fflush(stdout);
        } else if (!strncmp(line, "go", 2)) {
            SearchLimits lim;
            parse_go(&lim, line);
            Move best = search_bestmove(&CTX, &POS, &lim);
            char buf[8];
            if (best) move_to_uci(best, buf);
            else strcpy(buf, "0000");
            printf("bestmove %s\n", buf);
            fflush(stdout);
        } else if (!strncmp(line, "show", 4) || !strncmp(line, "display", 7)) {
            pos_print_pretty(&POS, LAST_FROM, LAST_TO);
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
