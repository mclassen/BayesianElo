#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <vector>

#include "parser/pgn_parser.h"
#include "rating/bayeselo_solver.h"
#include "output/terminal_output.h"
#include "output/export_writer.h"

using namespace bayeselo;

struct CliOptions {
    ParseOptions parse_options;
    FilterOptions filter_options;
    RatingOptions rating_options;
    std::optional<std::string> csv;
    std::optional<std::string> json;
    std::vector<std::string> files;
};

std::optional<std::uint32_t> parse_number(const std::string& arg) {
    if (arg.empty()) return std::nullopt;
    std::uint32_t value = 0;
    std::string suffix;
    std::string digits;
    for (char c : arg) {
        if (std::isdigit(static_cast<unsigned char>(c))) digits.push_back(c);
        else suffix.push_back(c);
    }
    if (digits.empty()) return std::nullopt;
    value = static_cast<std::uint32_t>(std::stoul(digits));
    if (suffix == "m") value *= 60;
    else if (suffix == "h") value *= 3600;
    return value;
}

void print_help() {
    std::cout << "Bayesian Elo CLI (inspired by BayesElo by Rémi Coulom)\n";
    std::cout << "Usage: elo_rating [options] files.pgn\n";
    std::cout << "--threads N, --min-plies N, --max-plies N, --min-time, --max-time, --white-name, --black-name\n";
}

CliOptions parse_args(int argc, char** argv) {
    CliOptions opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : std::string{}; };
        if (arg == "--threads") {
            opts.parse_options.threads = std::stoul(next());
        } else if (arg == "--chunk") {
            opts.parse_options.chunk_size = std::stoul(next());
        } else if (arg == "--min-plies") {
            opts.filter_options.min_plies = std::stoul(next());
        } else if (arg == "--max-plies") {
            opts.filter_options.max_plies = std::stoul(next());
        } else if (arg == "--min-moves") {
            opts.filter_options.min_plies = std::stoul(next()) * 2;
        } else if (arg == "--max-moves") {
            opts.filter_options.max_plies = std::stoul(next()) * 2;
        } else if (arg == "--min-time") {
            opts.filter_options.min_time = parse_number(next());
        } else if (arg == "--max-time") {
            opts.filter_options.max_time = parse_number(next());
        } else if (arg == "--white-name") {
            opts.filter_options.white_name = next();
        } else if (arg == "--black-name") {
            opts.filter_options.black_name = next();
        } else if (arg == "--either-name") {
            opts.filter_options.either_name = next();
        } else if (arg == "--exclude-name") {
            opts.filter_options.exclude_name = next();
        } else if (arg == "--result") {
            opts.filter_options.result = next();
        } else if (arg == "--termination") {
            opts.filter_options.termination = next();
        } else if (arg == "--require-complete") {
            opts.filter_options.require_complete = true;
        } else if (arg == "--skip-empty") {
            opts.filter_options.skip_empty = true;
        } else if (arg == "--anchor-player") {
            opts.rating_options.anchor_player = next();
        } else if (arg == "--anchor-rating") {
            opts.rating_options.anchor_rating = std::stod(next());
        } else if (arg == "--csv") {
            opts.csv = next();
        } else if (arg == "--json") {
            opts.json = next();
        } else if (arg == "--help") {
            print_help();
            std::exit(0);
        } else {
            if (arg.starts_with('-')) {
                std::cerr << "Unknown arg: " << arg << "\n";
            } else {
                opts.files.push_back(arg);
            }
        }
    }
    return opts;
}

int main(int argc, char** argv) {
    auto opts = parse_args(argc, argv);
    if (opts.files.empty()) {
        print_help();
        return 1;
    }

    GameFilter filter(opts.filter_options);
    PgnParser parser(opts.parse_options, filter);
    auto games = parser.parse_files(opts.files);

    BayesEloSolver solver(opts.rating_options);
    auto ratings = solver.solve(games);

    TerminalOutput term;
    term.print_summary(ratings);
    term.print_los(ratings);

    ExportWriter writer;
    if (opts.csv) writer.write_csv(*opts.csv, ratings);
    if (opts.json) writer.write_json(*opts.json, ratings);
    return 0;
}
