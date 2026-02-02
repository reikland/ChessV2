# ChessV2 C Engine

C11 chess engine with a stockfish-like architecture: bitboards, TT, iterative alpha-beta, UCI, and perft.

## Build

```sh
make
```

## Run

```sh
./engine
```

### UCI examples

```
uci
isready
position startpos
go depth 6
```

### Perft

```
position startpos
perft 4
```
