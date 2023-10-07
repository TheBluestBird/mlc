#pragma once

#include <list>
#include <string>
#include <array>
#include <string_view>
#include <algorithm>

class Loggable {
public:
    enum Severity {
        debug,
        info,
        minor,
        major,
        warning,
        error,
        fatal,
        _sevetirySize
    };
    typedef std::pair<Severity, std::string> Message;

    Loggable(Severity severity);
    ~Loggable();

    void log (Severity severity, const std::string& comment) const;
    std::list<Message> getHistory() const;

    static Severity stringToSeverity(const std::string& line);

private:
    const Severity currentSeverity;
    mutable std::list<Message> history;
};
