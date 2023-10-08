#pragma once

#include <list>
#include <string>

class Logger {
public:
    enum class Severity {
        debug,
        info,
        minor,
        major,
        warning,
        error,
        fatal,
        _sevetirySize
    };
    using Message = std::pair<Severity, std::string>;

    void debug(const std::string& comment, bool colored = true) const;
    void info(const std::string& comment, bool colored = true) const;
    void minor(const std::string& comment, bool colored = true) const;
    void major(const std::string& comment, bool colored = true) const;
    void warn(const std::string& comment, bool colored = true) const;
    void error(const std::string& comment, bool colored = true) const;
    void fatal(const std::string& comment, bool colored = true) const;

    virtual ~Logger();
    virtual void log(const std::list<Message>& comments, bool colored = true) const = 0;
    virtual void log(Severity severity, const std::string& comment, bool colored = true) const = 0;

    virtual void setSeverity(Severity severity) = 0;
    virtual Severity getSeverity() const = 0;

    static Severity stringToSeverity(const std::string& line);
};
