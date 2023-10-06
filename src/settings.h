#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <array>
#include <algorithm>

#include "loggable.h"

class Settings {
public:
    enum Action {
        convert,
        help,
        printConfig,
        _actionsSize
    };

    Settings(int argc, char **argv);

    std::string getInput() const;
    std::string getOutput() const;
    Loggable::Severity getLogLevel() const;
    Action getAction() const;

private:
    void parseArguments();

    static std::string_view stripFlags(const std::string_view& option);

private:
    std::vector<std::string_view> arguments;
    std::optional<Action> action;
    std::optional<std::string> input;
    std::optional<std::string> output;
    std::optional<Loggable::Severity> logLevel;
};
