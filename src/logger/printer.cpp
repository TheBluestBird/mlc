#include "printer.h"

#include <array>
#include <string_view>
#include <iostream>

constexpr const std::array<std::string_view, static_cast<int>(Logger::Severity::_severitySize)> logSettings({
    /*debug*/   "\e[90m",
    /*info*/    "\e[32m",
    /*minor*/   "\e[34m",
    /*major*/   "\e[94m",
    /*warning*/ "\e[33m",
    /*error*/   "\e[31m",
    /*fatal*/   "\e[91m"
});

constexpr const std::array<std::string_view, static_cast<int>(Logger::Severity::_severitySize)> logHeaders({
    /*debug*/   "DEBUG: ",
    /*info*/    "INFO: ",
    /*minor*/   "MINOR: ",
    /*major*/   "MAJOR: ",
    /*warning*/ "WARNING: ",
    /*error*/   "ERROR: ",
    /*fatal*/   "FATAL: "
});

constexpr const std::string_view bold("\e[1m");
constexpr const std::string_view regular("\e[22m");
constexpr const std::string_view clearStyle("\e[0m");
constexpr const std::string_view clearLine("\e[2K\r");

Printer::Printer(Severity severity):
    severity(severity),
    mutex(),
    status(std::nullopt)
{}

Logger::Severity Printer::getSeverity() const {
    return severity;
}

void Printer::setSeverity(Severity severity) {
    Printer::severity = severity;
}

void Printer::setStatusMessage(const std::string& message) {
    if (status.has_value())
        std::cout << clearLine;

    status = message;
    std::cout << message << std::flush;
}

void Printer::clearStatusMessage() {
    if (status.has_value()) {
        std::cout << clearLine << std::flush;
        status = std::nullopt;
    }
}

void Printer::printNested(
    const std::string& header,
    const std::vector<std::string>& lines,
    const std::list<Message>& comments,
    const std::optional<std::string>& status
) {
    std::lock_guard lock(mutex);
    if (comments.size() > 0) {
        if (status.has_value())
            std::cout << clearLine;

        std::cout << bold << header << clearStyle << "\n";
        for (const std::string& line : lines)
            std::cout << line << "\n";

        for (const Message& msg : comments) {
            if (msg.first >= severity) {
                std::cout << '\t';
                printMessage(msg, true);
            }
        }
    }

    if (status.has_value())
        Printer::status = status;

    finishPrint(true);
}

void Printer::log(const std::list<Message>& comments, bool colored) const {
    std::lock_guard lock(mutex);

    bool changed = false;
    for (const Message& msg : comments) {
        if (msg.first >= severity) {
            if (!changed && status.has_value())
                std::cout << clearLine;

            printMessage(msg, colored);
            changed = true;
        }
    }

    finishPrint(colored);
}

void Printer::log(Severity severity, const std::string& comment, bool colored) const {
    if (severity < Printer::severity)
        return;

    std::lock_guard lock(mutex);
    if (status.has_value())
        std::cout << clearLine;

    printMessage({severity, comment}, colored);
    finishPrint(colored);
}

void Printer::finishPrint(bool colored) const {
    if (colored)
        std::cout << clearStyle;

    if (status.has_value())
        std::cout << '\r' << status.value();

    std::cout << std::flush;
}

void Printer::printMessage(const Message& message, bool colored) const {
    if (colored) {
        int severity = static_cast<int>(message.first);
        std::cout << logSettings[severity] << bold << logHeaders[severity] << regular;
    }
    std::cout << message.second << '\n';
}
