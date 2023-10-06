#include "loggable.h"

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
