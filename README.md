# Bayesian Elo PGN CLI

A modern C++20 command-line utility that parses PGN files, applies composable filters, and estimates Bayesian Elo ratings inspired by Rémi Coulom's BayesElo.

> This tool is inspired by the original **BayesElo** rating system created by Rémi Coulom.
> Original project: [http://www.remi-coulom.fr/Bayesian-Elo](http://www.remi-coulom.fr/Bayesian-Elo)
>
> BayesElo established the mathematical and practical foundation for Bayesian rating systems. This tool is not affiliated with or endorsed by the original author.

## Building

```bash
cmake -S . -B build
cmake --build build
```

## Running

```bash
./build/elo_rating --threads 4 --min-plies 40 games/sample.pgn
```

Use `--help` for full CLI options.

## Testing

```bash
cmake -S . -B build
cmake --build build --target duration_tests parser_tests
ctest --test-dir build
```

