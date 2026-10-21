#include "logscan.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
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

} // namespace

std::vector<LogRecord> load_logs(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Could not open input file: " + path);
    }

    std::vector<LogRecord> logs;
    std::string line;

    while (std::getline(input, line)) {
        const auto fields = split_fields(line);
        if (fields.size() != 4) {
            continue;
        }

        LogRecord record;
        record.timestamp = fields[0];
        record.severity = fields[1];
        record.component = fields[2];
        record.message = fields[3];
        logs.push_back(record);
    }

    return logs;
}

SearchResult search_logs(const std::vector<LogRecord>& logs,
                         const std::string& severity,
                         const std::string& keyword) {
    SearchResult result;
    const std::string wanted_severity = lowercase_copy(severity);
    const std::string wanted_keyword = lowercase_copy(keyword);

    for (const auto& record : logs) {
        // Repeatedly creates lowercase copies for each record.
        const std::string current_severity = lowercase_copy(record.severity);
        const std::string current_message = lowercase_copy(record.message);

        const bool severity_matches =
            wanted_severity == "all" || current_severity == wanted_severity;
        const bool keyword_matches =
            wanted_keyword.empty() ||
            current_message.find(wanted_keyword) != std::string::npos;

        if (severity_matches && keyword_matches) {
            ++result.matched_records;
            result.message_bytes += record.message.size();
        }

        if (current_severity == "error") {
            ++result.error_records;
        }
        if (current_severity == "warning") {
            ++result.warning_records;
        }
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
