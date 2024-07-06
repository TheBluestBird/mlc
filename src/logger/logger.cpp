#include "logger.h"

#include <array>
#include <string_view>
#include <algorithm>

constexpr std::array<std::string_view, static_cast<int>(Logger::Severity::_severitySize)> levels({
        "debug",
        "info",
        "minor",
        "major",
        "warning",
        "error",
        "fatal"
});

Logger::~Logger() {}

void Logger::debug(const std::string& comment, bool colored) const {
    log(Severity::debug, comment, colored);
}

void Logger::info(const std::string& comment, bool colored) const {
    log(Severity::info, comment, colored);
}

void Logger::minor(const std::string& comment, bool colored) const {
    log(Severity::minor, comment, colored);
}

void Logger::major(const std::string& comment, bool colored) const {
    log(Severity::major, comment, colored);
}

void Logger::warn(const std::string& comment, bool colored) const {
    log(Severity::warning, comment, colored);
}

void Logger::error(const std::string& comment, bool colored) const {
    log(Severity::error, comment, colored);
}

void Logger::fatal(const std::string& comment, bool colored) const {
    log(Severity::fatal, comment, colored);
}

Logger::Severity Logger::stringToSeverity(const std::string& line) {
    unsigned char dist = std::distance(levels.begin(), std::find(levels.begin(), levels.end(), line));
    if (dist < static_cast<unsigned char>(Severity::_severitySize))
        return static_cast<Severity>(dist);

    return Severity::_severitySize;
}
