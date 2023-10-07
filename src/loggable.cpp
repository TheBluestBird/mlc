#include "loggable.h"

constexpr std::array<std::string_view, Loggable::_sevetirySize> levels({
        "debug",
        "info",
        "minor",
        "major",
        "warning",
        "error",
        "fatal"
});

Loggable::Loggable(Loggable::Severity severity):
    currentSeverity(severity),
    history()
{}

Loggable::~Loggable()
{}

void Loggable::log(Loggable::Severity severity, const std::string& comment) const {
    if (severity >= currentSeverity)
        history.emplace_back(severity, comment);
}

std::list<std::pair<Loggable::Severity, std::string>> Loggable::getHistory() const {
    return history;
}

Loggable::Severity Loggable::stringToSeverity(const std::string& line) {
    unsigned char dist = std::distance(levels.begin(), std::find(levels.begin(), levels.end(), line));
    if (dist < _sevetirySize)
        return static_cast<Severity>(dist);

    return _sevetirySize;
}
