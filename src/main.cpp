#include "logscan.hpp"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        std::cerr << "Usage:\n"
                  << "  logscan <file> <severity|ALL> <keyword>\n"
                  << "  logscan --generate <file> <record-count>\n";
        return 1;
    }

    if (std::string(argv[1]) == "--generate") {
        if (argc != 4) {
            std::cerr << "Usage: logscan --generate <file> <record-count>\n";
            return 1;
        }
        const auto count = static_cast<std::size_t>(std::strtoull(argv[3], nullptr, 10));
        generate_logs(argv[2], count);
        std::cout << "Generated " << count << " records in " << argv[2] << '\n';
        return 0;
    }

    const std::string path = argv[1];
    const std::string severity = argv[2];
    const std::string keyword = argv[3];

    const auto start = std::chrono::steady_clock::now();
    const auto logs = load_logs(path);
    const auto loaded = std::chrono::steady_clock::now();
    const auto result = search_logs(logs, severity, keyword);
    const auto finished = std::chrono::steady_clock::now();

    const auto load_ms =
        std::chrono::duration<double, std::milli>(loaded - start).count();
    const auto search_ms =
        std::chrono::duration<double, std::milli>(finished - loaded).count();

    std::cout << "records=" << logs.size() << '\n'
              << "matched=" << result.matched_records << '\n'
              << "errors=" << result.error_records << '\n'
              << "warnings=" << result.warning_records << '\n'
              << "message_bytes=" << result.message_bytes << '\n'
              << std::fixed << std::setprecision(3)
              << "load_ms=" << load_ms << '\n'
              << "search_ms=" << search_ms << '\n';
}
