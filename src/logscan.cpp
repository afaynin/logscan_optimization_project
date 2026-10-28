#include "logscan.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

std::vector<std::string> split_fields(const std::string& line) {
    // Intentionally simple: creates a new string for every field.
    std::vector<std::string> fields;
    std::size_t begin = 0;

    while (true) {
        const std::size_t end = line.find('|', begin);
        if (end == std::string::npos) {
            fields.push_back(line.substr(begin));
            break;
        }
        fields.push_back(line.substr(begin, end - begin));
        begin = end + 1;
    }
    return fields;
}

std::string lowercase_copy(std::string value) {
    // Copies the whole string even when the caller only needs to search it.
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });
    return value;
}

bool equals_lower(const std::string& s, const std::string& lower_word) {
    if (s.size() != lower_word.size()) return false;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (std::tolower(s[i]) != lower_word[i]) return false;
    }
    return true;
}

bool contains_lower(const std::string& s, const std::string& lower_word) {
    if (lower_word.size() > s.size()) return false;
    for (std::size_t start = 0; start + lower_word.size() <= s.size(); ++start) {
        std::size_t i = 0;
        while (i < lower_word.size() &&
               std::tolower(s[start + i]) == lower_word[i]) {
            ++i;
        }
        if (i == lower_word.size()) return true;
    }
    return false;
}

} // namespace

std::vector<LogRecord> load_logs(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Could not open input file: " + path);
    }

    std::vector<LogRecord> logs;
    std::string line;

    while (std::getline(input, line)) {
        auto fields = split_fields(line);
        if (fields.size() != 4) {
            continue;
        }

        LogRecord record;
        record.timestamp = std::move(fields[0]);
        record.severity = std::move(fields[1]);
        record.component = std::move(fields[2]);
        record.message = std::move(fields[3]);
        logs.push_back(std::move(record));
    }

    return logs;
}

SearchResult search_range(const std::vector<LogRecord>& logs,
                          int begin, int end,
                          const std::string& wanted_severity,
                          const std::string& wanted_keyword) {
    SearchResult result;

    const std::string error_word = "error";
    const std::string warning_word = "warning";
    for (int i = begin; i < end; ++i) {
        const auto& record = logs[i];
        // // Repeatedly creates lowercase copies for each record.
        // const std::string current_severity = lowercase_copy(record.severity);
        // const std::string current_message = lowercase_copy(record.message);
        const bool severity_matches =
            wanted_severity == "all" || equals_lower(record.severity, wanted_severity);
        const bool keyword_matches =
            wanted_keyword.empty() || contains_lower(record.message, wanted_keyword);

        if (severity_matches && keyword_matches) {
            ++result.matched_records;
            result.message_bytes += record.message.size();
        }

        if (equals_lower(record.severity, error_word)) {
            ++result.error_records;
        }
        if (equals_lower(record.severity, warning_word)) {
            ++result.warning_records;
        }
    }

    return result;
}

SearchResult search_logs(const std::vector<LogRecord>& logs,
                         const std::string& severity,
                         const std::string& keyword) {
    const std::string wanted_severity = lowercase_copy(severity);
    const std::string wanted_keyword = lowercase_copy(keyword);

    const int n = 8;
    const int count =logs.size();

    SearchResult partial[n];
    std::thread threads[n];

    for (int t = 0; t < n; ++t) {
        const int begin = count * t / n;
        const int end = count * (t + 1) / n;
        threads[t] = std::thread([&, t, begin, end] {
            partial[t] = search_range(logs, begin, end, wanted_severity, wanted_keyword);
        });
    }

    SearchResult result;
    for (int t = 0; t < n; ++t) {
        threads[t].join();
        result.matched_records += partial[t].matched_records;
        result.error_records += partial[t].error_records;
        result.warning_records += partial[t].warning_records;
        result.message_bytes += partial[t].message_bytes;
    }
    return result;
}

void generate_logs(const std::string& path, std::size_t count) {
    static const char* severities[] = {"INFO", "DEBUG", "WARNING", "ERROR"};
    static const char* components[] = {
        "network", "storage", "scheduler", "allocator", "parser"
    };
    static const char* messages[] = {
        "connection established with peer",
        "request queue reached configured limit",
        "cache lookup completed successfully",
        "allocation failed while reserving buffer",
        "retrying operation after transient timeout",
        "worker thread entered idle state",
        "configuration reload completed",
        "disk write returned an unexpected status",
        "packet received but checksum was invalid",
        "operation completed in acceptable time"
    };

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Could not create output file: " + path);
    }

    // Deterministic data makes before/after benchmarks reproducible.
    for (std::size_t i = 0; i < count; ++i) {
        output << "2026-10-21T12:"
               << (i / 60 % 60) << ':'
               << (i % 60) << '|'
               << severities[i % 4] << '|'
               << components[(i / 3) % 5] << '|'
               << messages[(i * 7 + i / 11) % 10]
               << " request_id=" << (i % 10000)
               << " sequence=" << i << '\n';
    }
}
