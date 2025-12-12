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

Memory controls:
- `--max-games N` caps the number of filtered games kept in memory (extra parsed games are discarded).
- `--max-size <bytes|k|m|g>` caps approximate retained memory (e.g., `1G`, `512m`, `4096`).
- `--keep-moves` preserves full move text; by default moves are dropped after counting plies to save memory and use the compact pairing path.

Benchmark helper (disabled in ctest by default; run manually if desired):
```bash
./build/bench_parser --generate-pgn-size=8G --chunk-size=64M --keep-file
# Env vars also supported: BENCH_PGN_MB=8000 BENCH_CHUNK_BYTES=64 BENCH_KEEP_FILE=1
```
This generates a synthetic PGN (with varied results/terminations/time controls), measures parsing throughput, and optionally keeps the file for further testing.

## Testing

```bash
cmake -S . -B build
cmake --build build --target duration_tests parser_tests
ctest --test-dir build
```
