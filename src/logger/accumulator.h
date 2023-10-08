#pragma once

#include "logger.h"

class Accumulator : public Logger {
public:
    Accumulator(Severity severity = Severity::info);

    virtual void log(const std::list<Message>& comments, bool colored = true) const override;
    virtual void log(Severity severity, const std::string& comment, bool colored = true) const override;

    virtual Severity getSeverity() const override;
    virtual void setSeverity(Severity severity) override;

    std::list<Message> getHistory() const;

private:
    Severity severity;
    mutable std::list<Message> history;
};
