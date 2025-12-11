#include "bayeselo/duration.h"
#include "bayeselo/filters.h"
#include "bayeselo/game.h"
#include "bayeselo/rating_result.h"
#include "output/export_writer.h"
#include "output/terminal_output.h"
#include "parser/chunk_splitter.h"
#include "parser/pgn_parser.h"
#include "rating/bayeselo_solver.h"
#include "util/thread_pool.h"

#include <filesystem>
#include <format>
#include <iostream>
#include <algorithm>
#include <latch>
#include <mutex>
#include <queue>
#include <ranges>
#include <sstream>

using namespace bayeselo;

struct CliOptions {
    std::vector<std::filesystem::path> files;
    FilterConfig filters;
    std::optional<std::filesystem::path> csv;
    std::optional<std::filesystem::path> json;
    std::size_t threads{std::thread::hardware_concurrency()};
};

void print_help() {
    std::cout << "Bayesian Elo PGN rating tool\n";
    std::cout << "Inspired by BayesElo by Rémi Coulom (http://www.remi-coulom.fr/Bayesian-Elo)\n";
    std::cout << "Usage: elo_rating [options] file1.pgn file2.pgn ...\n";
}

CliOptions parse_cli(int argc, char** argv) {
    CliOptions options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help();
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

    ThreadPool pool(options.threads);
    std::mutex games_mutex;
    std::vector<Game> games;
    std::latch latch(options.files.size());

    for (const auto& file : options.files) {
        auto chunks = split_pgn_file(file, 1 << 20);
        if (chunks.empty()) {
            latch.count_down();
            continue;
        }
        for (const auto& chunk : chunks) {
            pool.enqueue([&, chunk]() {
                auto parsed = parse_pgn_chunk(chunk.file, chunk.start_offset, chunk.end_offset);
                std::scoped_lock lock(games_mutex);
                games.insert(games.end(), parsed.begin(), parsed.end());
            });
        }
        latch.count_down();
    }

    latch.wait();
    pool.shutdown();

    std::vector<Game> filtered;
    std::ranges::copy_if(games, std::back_inserter(filtered), [&](const Game& g) { return passes_filters(g, options.filters); });

    BayesEloSolver solver;
    auto ratings = solver.solve(filtered);

    print_ratings(ratings);
    if (options.csv) write_csv(ratings, *options.csv);
    if (options.json) write_json(ratings, *options.json);
    return 0;
}

