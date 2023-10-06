#pragma once

#include <list>
#include <string>

class Loggable {
public:
    enum Severity {
        debug,
        info,
        minor,
        major,
        warning,
        error,
        fatal
    };
    typedef std::pair<Severity, std::string> Message;

    Loggable(Severity severity);
    ~Loggable();

    void log (Severity severity, const std::string& comment) const;
    std::list<Message> getHistory() const;

private:
    const Severity currentSeverity;
    mutable std::list<Message> history;
};
