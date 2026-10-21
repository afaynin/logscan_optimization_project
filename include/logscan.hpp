#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct LogRecord {
    std::string timestamp;
    std::string severity;
    std::string component;
    std::string message;
};

struct SearchResult {
    std::size_t matched_records = 0;
    std::size_t error_records = 0;
    std::size_t warning_records = 0;
    std::size_t message_bytes = 0;
};

std::vector<LogRecord> load_logs(const std::string& path);
SearchResult search_logs(const std::vector<LogRecord>& logs,
                         const std::string& severity,
                         const std::string& keyword);
void generate_logs(const std::string& path, std::size_t count);
