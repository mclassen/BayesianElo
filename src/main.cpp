#include "bayeselo/duration.h"
#include "bayeselo/filters.h"
#include "bayeselo/game.h"
#include "bayeselo/rating_result.h"
#include "bayeselo/size_parse.h"
#include "version.h"
#include "output/export_writer.h"
#include "output/terminal_output.h"
#include "parser/chunk_splitter.h"
#include "parser/pgn_parser.h"
#include "rating/bayeselo_solver.h"
#include "util/thread_pool.h"

#include <atomic>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <vector>

using namespace bayeselo;

struct CliOptions {
    std::vector<std::filesystem::path> files;
    FilterConfig filters;
    std::optional<std::filesystem::path> csv;
    std::optional<std::filesystem::path> json;
    std::size_t threads{std::thread::hardware_concurrency()};
    std::optional<std::size_t> max_games;
    bool keep_moves{false};
    std::optional<std::size_t> max_bytes;
};

void print_help() {
    std::cout
        << "Bayesian Elo PGN rating tool\n"
        << "Inspired by BayesElo by Rémi Coulom (http://www.remi-coulom.fr/Bayesian-Elo)\n"
        << "Usage: elo_rating [options] file1.pgn file2.pgn ...\n\n"
        << "Options:\n"
        << "  -h, --help                  Show this help message and exit\n"
        << "  --version                   Print version information and exit\n"
        << "  --threads <n>               Number of worker threads (default: hardware concurrency)\n"
        << "  --csv <path>                Write ratings table as CSV\n"
        << "  --json <path>               Write ratings table as JSON\n"
        << "  --max-games <n>             Stop after N accepted games\n"
        << "  --max-size <bytes|k|m|g>    Cap approximate memory for names + pairings\n"
        << "  --keep-moves                Retain SAN move text (otherwise dropped after ply counting)\n"
        << "\nFilters:\n"
        << "  --min-plies <n>             Minimum plies (half-moves)\n"
        << "  --max-plies <n>             Maximum plies (half-moves)\n"
        << "  --min-moves <n>             Minimum moves (converted to plies)\n"
        << "  --max-moves <n>             Maximum moves (converted to plies)\n"
        << "  --min-time <dur>            Minimum duration; accepts seconds or suffix h/m/s (e.g. 300, 5m, 1h)\n"
        << "  --max-time <dur>            Maximum duration; \"300+2\" keeps the base time (300)\n"
        << "  --white-name <substr>       Require White name contains substring\n"
        << "  --black-name <substr>       Require Black name contains substring\n"
        << "  --either-name <substr>      Require either name contains substring\n"
        << "  --exclude-name <substr>     Exclude games if either name contains substring\n"
        << "  --result <1-0|0-1|1/2-1/2>  Filter by result\n"
        << "  --termination <value>       Filter by Termination tag (case-insensitive)\n"
        << "  --require-complete          Skip games missing required metadata/result\n"
        << "  --skip-empty                Skip games with empty/unknown result\n"
        << "\nNotes:\n"
        << "  - Provide one or more PGN files to rate. Games are filtered before rating.\n"
        << "  - Size suffixes: k=KiB, m=MiB, g=GiB. Duration suffixes: s, m, h.\n"
        << "  - When --keep-moves is omitted, compact pairings are used for lower memory.\n";
}

CliOptions parse_cli(int argc, char** argv) {
    CliOptions options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help();
            std::exit(0);
        } else if (arg == "--version") {
            std::cout << "Bayesian Elo PGN CLI " << BAYESELO_VERSION_STRING
                      << " (" << BAYESELO_GIT_HASH << ")\n";
            std::exit(0);
        } else if (arg == "--threads" && i + 1 < argc) {
            options.threads = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--min-plies" && i + 1 < argc) {
            options.filters.min_plies = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--max-plies" && i + 1 < argc) {
            options.filters.max_plies = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--min-moves" && i + 1 < argc) {
            options.filters.min_plies = static_cast<std::uint32_t>(std::stoul(argv[++i]) * 2);
        } else if (arg == "--max-moves" && i + 1 < argc) {
            options.filters.max_plies = static_cast<std::uint32_t>(std::stoul(argv[++i]) * 2);
        } else if (arg == "--min-time" && i + 1 < argc) {
            options.filters.min_time_seconds = parse_duration_to_seconds(argv[++i]);
        } else if (arg == "--max-time" && i + 1 < argc) {
            options.filters.max_time_seconds = parse_duration_to_seconds(argv[++i]);
        } else if (arg == "--white-name" && i + 1 < argc) {
            options.filters.white_name = argv[++i];
        } else if (arg == "--black-name" && i + 1 < argc) {
            options.filters.black_name = argv[++i];
        } else if (arg == "--either-name" && i + 1 < argc) {
            options.filters.either_name = argv[++i];
        } else if (arg == "--exclude-name" && i + 1 < argc) {
            options.filters.exclude_name = argv[++i];
        } else if (arg == "--result" && i + 1 < argc) {
            options.filters.result_filter = argv[++i];
        } else if (arg == "--termination" && i + 1 < argc) {
            options.filters.termination = argv[++i];
        } else if (arg == "--require-complete") {
            options.filters.require_complete = true;
        } else if (arg == "--skip-empty") {
            options.filters.skip_empty = true;
        } else if (arg == "--csv" && i + 1 < argc) {
            options.csv = argv[++i];
        } else if (arg == "--json" && i + 1 < argc) {
            options.json = argv[++i];
        } else if (arg == "--max-games" && i + 1 < argc) {
            options.max_games = static_cast<std::size_t>(std::stoull(argv[++i]));
        } else if (arg == "--keep-moves") {
            options.keep_moves = true;
        } else if (arg == "--max-size" && i + 1 < argc) {
            options.max_bytes = parse_size(argv[++i]);
        } else if (!arg.empty() && arg.front() != '-') {
            options.files.push_back(arg);
        }
    }
    return options;
}

int main(int argc, char** argv) {
    auto options = parse_cli(argc, argv);
    if (options.files.empty()) {
        print_help();
        return 1;
    }

    std::vector<ChunkRange> chunks;
    chunks.reserve(options.files.size());
    for (const auto& file : options.files) {
        auto ranges = split_pgn_file(file, 1 << 20);
        chunks.insert(chunks.end(), ranges.begin(), ranges.end());
    }

    ThreadPool pool(options.threads);
    std::mutex games_mutex;
    std::vector<Game> games;
    std::atomic_size_t accepted{0};
    std::atomic_bool max_reached{false};
    std::vector<Pairing> pairings;
    std::vector<std::string> player_names;
    std::unordered_map<std::string, std::size_t> name_index;
    std::atomic_size_t estimated_bytes{0};
    const bool use_pairings = !options.keep_moves;
    constexpr std::size_t pairing_bytes = sizeof(Pairing);
    constexpr std::size_t name_overhead = sizeof(std::string);

    for (const auto& chunk : chunks) {
        pool.enqueue([&, chunk]() {
            auto parsed = parse_pgn_chunk(chunk.file, chunk.start_offset, chunk.end_offset);
            std::vector<Game> local_games;
            std::vector<Pairing> local_pairs;
            local_games.reserve(parsed.size());
            local_pairs.reserve(parsed.size());

            auto try_accept_game = [&]() -> bool {
                if (!options.max_games) {
                    accepted.fetch_add(1, std::memory_order_relaxed);
                    return true;
                }
                std::size_t current = accepted.load(std::memory_order_relaxed);
                while (true) {
                    if (current >= *options.max_games) {
                        return false;
                    }
                    if (accepted.compare_exchange_weak(current, current + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                        return true;
                    }
                }
            };

            for (auto& g : parsed) {
                if (!options.keep_moves) {
                    g.moves.clear();
                    g.moves.shrink_to_fit();
                }
                if (!passes_filters(g, options.filters)) continue;

                if (options.max_games && accepted.load(std::memory_order_relaxed) >= *options.max_games) {
                    max_reached.store(true, std::memory_order_relaxed);
                    break;
                }

                if (use_pairings) {
                    if (g.result.outcome == GameResult::Outcome::Unknown) continue;
                    std::size_t w_idx, b_idx;
                    {
                        std::scoped_lock lock(games_mutex);
                        auto itw = name_index.find(g.meta.white);
                        if (itw == name_index.end()) {
                            w_idx = player_names.size();
                            name_index[g.meta.white] = w_idx;
                            player_names.push_back(g.meta.white);
                            estimated_bytes.fetch_add(g.meta.white.size() + name_overhead, std::memory_order_relaxed);
                        } else {
                            w_idx = itw->second;
                        }
                        auto itb = name_index.find(g.meta.black);
                        if (itb == name_index.end()) {
                            b_idx = player_names.size();
                            name_index[g.meta.black] = b_idx;
                            player_names.push_back(g.meta.black);
                            estimated_bytes.fetch_add(g.meta.black.size() + name_overhead, std::memory_order_relaxed);
                        } else {
                            b_idx = itb->second;
                        }
                    }
                    double score = 0.5;
                    if (g.result.outcome == GameResult::Outcome::WhiteWin) score = 1.0;
                    else if (g.result.outcome == GameResult::Outcome::BlackWin) score = 0.0;

                    if (options.max_bytes) {
                        auto current = estimated_bytes.load(std::memory_order_relaxed);
                        bool limit_hit = false;
                        while (true) {
                            if (current + pairing_bytes > *options.max_bytes) {
                                max_reached.store(true, std::memory_order_relaxed);
                                limit_hit = true;
                                break;
                            }
                            if (estimated_bytes.compare_exchange_weak(current, current + pairing_bytes,
                                                                      std::memory_order_relaxed,
                                                                      std::memory_order_relaxed)) {
                                break;
                            }
                        }
                        if (limit_hit) {
                            break;
                        }
                    }

                    if (!try_accept_game()) {
                        max_reached.store(true, std::memory_order_relaxed);
                        break;
                    }

                    local_pairs.push_back(Pairing{w_idx, b_idx, score});
                } else {
                    if (!try_accept_game()) {
                        max_reached.store(true, std::memory_order_relaxed);
                        break;
                    }
                    local_games.push_back(std::move(g));
                }
            }

            if (!local_games.empty() || !local_pairs.empty()) {
                std::scoped_lock lock(games_mutex);
                if (!local_games.empty()) {
                    games.insert(games.end(),
                                 std::make_move_iterator(local_games.begin()),
                                 std::make_move_iterator(local_games.end()));
                }
                if (!local_pairs.empty()) {
                    pairings.insert(pairings.end(),
                                    std::make_move_iterator(local_pairs.begin()),
                                    std::make_move_iterator(local_pairs.end()));
                }
            }
        });
    }

    pool.wait_for_completion();
    pool.shutdown();

    BayesEloSolver solver;
    RatingResult ratings;
    if (use_pairings) {
        ratings = solver.solve(pairings, player_names);
    } else {
        ratings = solver.solve(games);
    }

    print_ratings(ratings);
    if (max_reached.load(std::memory_order_relaxed)) {
        std::cerr << "Reached limit (--max-games or --max-bytes); remaining parsed games were discarded.\n";
    }
    if (options.csv) write_csv(ratings, *options.csv);
    if (options.json) write_json(ratings, *options.json);
    return 0;
}
