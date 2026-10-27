#include "core/sentinel_scanner.h"

#include <utility>

//Constructor
SentinelScanner::SentinelScanner(std::string sentinel)
    : sentinel_(std::move(sentinel)) {}

//Feed chunk
SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    std::string combined = pending_;
    combined.append(chunk.data(), chunk.size());
    pending_.clear();

    //Find sentinel
    const std::size_t found = combined.find(sentinel_);
    if (found != std::string::npos) {
        return {combined.substr(0, found), true};
    }

    //Hold possible sentinel prefix
    const std::size_t hold_back = sentinel_.size() - 1;
    if (combined.size() <= hold_back) {
        pending_ = std::move(combined);
        return {std::string{}, false};
    }

    //Return safe text
    const std::size_t safe_count = combined.size() - hold_back;
    std::string safe_text = combined.substr(0, safe_count);
    pending_ = combined.substr(safe_count);
    return {std::move(safe_text), false};
}

//Flush pending text
SentinelScanner::Out SentinelScanner::flush() {
    std::string safe_text = std::move(pending_);
    pending_.clear();
    return {std::move(safe_text), false};
}
