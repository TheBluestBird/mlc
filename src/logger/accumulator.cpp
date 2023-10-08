#include "accumulator.h"

Accumulator::Accumulator(Severity severity):
    severity(severity),
    history()
{}

Logger::Severity Accumulator::getSeverity() const {
    return severity;
}

void Accumulator::setSeverity(Severity severity) {
    Accumulator::severity = severity;
}

void Accumulator::log(const std::list<Message>& comments, bool colored) const {
    (void)(colored);
    for (const Message& comment : comments)
        if (comment.first >= Accumulator::severity)
            history.emplace_back(comment);
}

void Accumulator::log(Severity severity, const std::string& comment, bool colored) const {
    (void)(colored);
    if (severity < Accumulator::severity)
        return;

    history.emplace_back(severity, comment);
}

std::list<Logger::Message> Accumulator::getHistory() const {
    return history;
}
