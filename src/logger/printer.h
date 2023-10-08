#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <optional>
#include <vector>

#include "logger.h"

class Printer : public Logger {
public:
    Printer(Severity severity = Severity::info);

    virtual void log(const std::list<Message>& comments, bool colored = true) const override;
    virtual void log(Severity severity, const std::string& comment, bool colored = true) const override;

    virtual void setSeverity(Severity severity) override;
    virtual Severity getSeverity() const override;

    void setStatusMessage(const std::string& message);
    void clearStatusMessage();
    void printNested(
        const std::string& header,
        const std::vector<std::string>& lines = {},
        const std::list<Message>& comments = {},
        const std::optional<std::string>& status = std::nullopt
    );

private:
    void printMessage(const Message& message, bool colored = false) const;
    void finishPrint(bool colored = false) const;

private:
    std::atomic<Severity> severity;
    mutable std::mutex mutex;
    std::optional<std::string> status;
};
