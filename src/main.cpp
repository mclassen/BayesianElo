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
#include <limits>
#include <mutex>
#include <optional>
#include <thread>
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
        << "  --max-size <bytes|k|m|g>    Cap approximate total memory for names, pairings, and internal overhead\n"
        << "  --keep-moves                Retain SAN move text (otherwise dropped after ply counting)\n"
        << "\nFilters:\n"
        << "  --min-plies <n>             Minimum plies (half-moves)\n"
        << "  --max-plies <n>             Maximum plies (half-moves)\n"
        << "  --min-moves <n>             Minimum moves (converted to plies)\n"
        << "  --max-moves <n>             Maximum moves (converted to plies)\n"
        << "  --min-time <dur>            Minimum duration; accepts seconds or suffix h/m/s (e.g. 300, 5m, 1h)\n"
        << "  --max-time <dur>            Maximum duration; e.g. \"300+2\" uses only the base time (300); increments are ignored\n"
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
        << "  - When --keep-moves is omitted, moves are discarded after ply counting and only compact pairings/results are retained, reducing memory.\n"
        << "  - Use --keep-moves if you plan to export move text or perform move-level analysis later.\n";
}

CliOptions parse_cli(int argc, char** argv) {
    CliOptions options;
    auto require_value = [&](const std::string& opt, int i) {
        if (i + 1 >= argc) {
            std::cerr << opt << " requires a value\n";
            return false;
        }
        return true;
    };
    auto parse_size_t_arg = [&](const std::string& opt, int& i, std::size_t& out) {
        if (!require_value(opt, i)) {
            return false;
        }
        const char* val = argv[++i];
        try {
            out = static_cast<std::size_t>(std::stoull(val));
        } catch (const std::exception&) {
            std::cerr << "Invalid value for " << opt << ": " << val << "\n";
            return false;
        }
        return true;
    };
    auto parse_uint32_arg = [&](const std::string& opt, int& i, std::uint32_t& out) {
        if (!require_value(opt, i)) {
            return false;
        }
        const char* val = argv[++i];
        try {
            unsigned long v = std::stoul(val);
            if (v > std::numeric_limits<std::uint32_t>::max()) {
                throw std::out_of_range("too large");
            }
            out = static_cast<std::uint32_t>(v);
        } catch (const std::exception&) {
            std::cerr << "Invalid value for " << opt << ": " << val << "\n";
            return false;
        }
        return true;
    };
    auto parse_duration_arg = [&](const std::string& opt, int& i, std::optional<double>& out) {
        if (!require_value(opt, i)) {
            return false;
        }
        const char* val = argv[++i];
        try {
            out = parse_duration_to_seconds(val);
        } catch (const std::exception& ex) {
            std::cerr << "Invalid value for " << opt << ": " << val << " (" << ex.what() << ")\n";
            return false;
        }
        return true;
    };
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help();
            std::exit(0);
        }
        if (arg == "--version") {
            std::cout << "Bayesian Elo PGN CLI " << BAYESELO_VERSION_STRING
                      << " (" << BAYESELO_GIT_HASH << ")\n";
            std::exit(0);
        }
        if (arg == "--threads") {
            std::size_t threads = 0;
            if (!parse_size_t_arg(arg, i, threads)) {
                std::cerr << "Invalid value for --threads: expected integer in [1,1024]\n";
                std::exit(1);
            }
            if (threads == 0 || threads > 1024) {
                std::cerr << "--threads must be in [1,1024]\n";
                std::exit(1);
            }
            options.threads = threads;
            continue;
        }
        if (arg == "--min-plies") {
            std::uint32_t v = 0;
            if (!parse_uint32_arg(arg, i, v)) {
                std::exit(1);
            }
            options.filters.min_plies = v;
            continue;
        }
        if (arg == "--max-plies") {
            std::uint32_t v = 0;
            if (!parse_uint32_arg(arg, i, v)) {
                std::exit(1);
            }
            options.filters.max_plies = v;
            continue;
        }
        if (arg == "--min-moves") {
            std::uint32_t v = 0;
            if (!parse_uint32_arg(arg, i, v)) {
                std::exit(1);
            }
            options.filters.min_plies = v * 2;
            continue;
        }
        if (arg == "--max-moves") {
            std::uint32_t v = 0;
            if (!parse_uint32_arg(arg, i, v)) {
                std::exit(1);
            }
            options.filters.max_plies = v * 2;
            continue;
        }
        if (arg == "--min-time") {
            if (!parse_duration_arg(arg, i, options.filters.min_time_seconds)) {
                std::exit(1);
            }
            continue;
        }
        if (arg == "--max-time") {
            if (!parse_duration_arg(arg, i, options.filters.max_time_seconds)) {
                std::exit(1);
            }
            continue;
        }
        if (arg == "--white-name") {
            if (!require_value(arg, i)) {
                std::exit(1);
            }
            options.filters.white_name = argv[++i];
            continue;
        }
        if (arg == "--black-name") {
            if (!require_value(arg, i)) {
                std::exit(1);
            }
            options.filters.black_name = argv[++i];
            continue;
        }
        if (arg == "--either-name") {
            if (!require_value(arg, i)) {
                std::exit(1);
            }
            options.filters.either_name = argv[++i];
            continue;
        }
        if (arg == "--exclude-name") {
            if (!require_value(arg, i)) {
                std::exit(1);
            }
            options.filters.exclude_name = argv[++i];
            continue;
        }
        if (arg == "--result") {
            if (!require_value(arg, i)) {
                std::exit(1);
            }
            options.filters.result_filter = argv[++i];
            continue;
        }
        if (arg == "--termination") {
            if (!require_value(arg, i)) {
                std::exit(1);
            }
            options.filters.termination = argv[++i];
            continue;
        }
        if (arg == "--require-complete") {
            options.filters.require_complete = true;
            continue;
        }
        if (arg == "--skip-empty") {
            options.filters.skip_empty = true;
            continue;
        }
        if (arg == "--csv") {
            if (!require_value(arg, i)) {
                std::exit(1);
            }
            options.csv = argv[++i];
            continue;
        }
        if (arg == "--json") {
            if (!require_value(arg, i)) {
                std::exit(1);
            }
            options.json = argv[++i];
            continue;
        }
        if (arg == "--max-games") {
            std::size_t v = 0;
            if (!parse_size_t_arg(arg, i, v)) {
                std::exit(1);
            }
            options.max_games = v;
            continue;
        }
        if (arg == "--keep-moves") {
            options.keep_moves = true;
            continue;
        }
        if (arg == "--max-size") {
            if (!require_value(arg, i)) {
                std::exit(1);
            }
            options.max_bytes = parse_size(argv[++i]);
            if (!options.max_bytes) {
                std::cerr << "Invalid value for --max-size: " << argv[i] << "\n";
                std::exit(1);
            }
            continue;
        }
        if (!arg.empty() && arg.front() != '-') {
            options.files.push_back(arg);
            continue;
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

    // 1 MiB chunks: large enough to amortize file I/O overhead, small enough to keep parallelism granular.
    constexpr std::size_t default_chunk_bytes = 1u << 20;
    std::vector<ChunkRange> chunks;
    chunks.reserve(options.files.size());
    for (const auto& file : options.files) {
        auto ranges = split_pgn_file(file, default_chunk_bytes);
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
    constexpr std::size_t pairing_bytes = sizeof(Pairing); // Heuristic; we intentionally avoid extra margins to keep limits intuitive (see PR discussion).
    constexpr std::size_t name_overhead = sizeof(std::string); // Same here: this tracks control blocks only so --max-size is a soft cap by design.
    auto reserve_bytes = [&](std::size_t bytes) -> bool {
        if (!options.max_bytes) {
            return true;
        }
        int attempts = 0;
        while (true) {
            auto current = estimated_bytes.load(std::memory_order_acquire);
            auto next = current + bytes;
            if (next > *options.max_bytes) {
                max_reached.store(true, std::memory_order_relaxed);
                return false;
            }
            if (estimated_bytes.compare_exchange_weak(current, next,
                                                      std::memory_order_acq_rel,
                                                      std::memory_order_acquire)) {
                return true;
            }
            if (++attempts % 8 == 0) {
                std::this_thread::yield(); // Simple spin/yield was chosen over heavier primitives; see PR thread.
            }
        }
    };

    for (const auto& chunk : chunks) {
        pool.enqueue([&, chunk]() {
            auto parsed = parse_pgn_chunk(chunk.file, chunk.start_offset, chunk.end_offset);
            std::vector<Game> local_games;
            std::vector<Pairing> local_pairs;
            if (!parsed) {
                std::cerr << "Failed to parse chunk: " << chunk.file
                          << " (offsets " << chunk.start_offset << "-" << chunk.end_offset << ")\n";
                return;
            }
            local_games.reserve(parsed->size());
            local_pairs.reserve(parsed->size());

            auto try_accept_game = [&]() -> bool {
                if (!options.max_games) {
                    accepted.fetch_add(1, std::memory_order_relaxed);
                    return true;
                }
                int attempts = 0;
                std::size_t current = accepted.load(std::memory_order_acquire);
                while (true) {
                    if (current >= *options.max_games) {
                        return false;
                    }
                    if (accepted.compare_exchange_weak(current, current + 1, std::memory_order_acq_rel, std::memory_order_acquire)) {
                        return true;
                    }
                    if (++attempts % 8 == 0) {
                        std::this_thread::yield(); // Same deliberate policy as reserve_bytes: lightweight spin + yield.
                    }
                }
            };

            for (auto& g : *parsed) {
                if (!options.keep_moves) {
                    g.moves.clear();
                    g.moves.shrink_to_fit();
                }
                if (!passes_filters(g, options.filters)) continue;

                if (use_pairings) {
                    if (g.result.outcome == GameResult::Outcome::Unknown) continue;
                    std::size_t w_idx, b_idx;
                    {
                        std::scoped_lock lock(games_mutex);
                        auto itw = name_index.find(g.meta.white);
                        if (itw == name_index.end()) {
                            if (!reserve_bytes(g.meta.white.size() + name_overhead)) {
                                break;
                            }
                            w_idx = player_names.size();
                            name_index[g.meta.white] = w_idx;
                            player_names.push_back(g.meta.white);
                        } else {
                            w_idx = itw->second;
                        }
                        auto itb = name_index.find(g.meta.black);
                        if (itb == name_index.end()) {
                            if (!reserve_bytes(g.meta.black.size() + name_overhead)) {
                                break;
                            }
                            b_idx = player_names.size();
                            name_index[g.meta.black] = b_idx;
                            player_names.push_back(g.meta.black);
                        } else {
                            b_idx = itb->second;
                        }
                    }
                    double score = 0.5;
                    if (g.result.outcome == GameResult::Outcome::WhiteWin) score = 1.0;
                    else if (g.result.outcome == GameResult::Outcome::BlackWin) score = 0.0;
                    if (!reserve_bytes(pairing_bytes)) {
                        break;
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
        std::cerr << "Reached limit (--max-games or --max-size); remaining parsed games were discarded.\n";
    }
    if (options.csv) write_csv(ratings, *options.csv);
    if (options.json) write_json(ratings, *options.json);
    return 0;
}
