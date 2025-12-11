# Bayesian Elo CLI

A modern C++20 command-line tool that parses PGN files, applies flexible filters, and computes Bayesian Elo ratings with LOS output. Inspired by BayesElo by Rémi Coulom.

## Features
- Chunked PGN parsing without regular expressions
- Adaptive thread pool using `std::jthread`, `std::stop_token`, `std::latch`, and `std::ranges`
- Game filtering by length, duration, player names, results, and termination
- Bayesian Elo solver with LOS matrix output and optional rating anchors
- Colorized terminal summary plus CSV/JSON export

## Attribution
This tool is inspired by the original **BayesElo** rating system created by Rémi Coulom. Original project: [http://www.remi-coulom.fr/Bayesian-Elo](http://www.remi-coulom.fr/Bayesian-Elo)

BayesElo established the mathematical and practical foundation for Bayesian rating systems. This tool is not affiliated with or endorsed by the original author.

## Building
```bash
cmake -S . -B build
cmake --build build
```

## Running
```bash
./build/elo_rating --threads 4 --min-plies 40 --csv ratings.csv games/sample.pgn
```

## Testing
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```
