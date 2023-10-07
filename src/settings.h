#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <functional>
#include <cctype>
#include <sstream>

#include "loggable.h"

class Settings {
public:
    enum Action {
        convert,
        help,
        config,
        _actionsSize
    };

    enum Type {
        mp3,
        _typesSize
    };

    enum Option {
        level,
        type,
        _optionsSize
    };

    Settings(int argc, char **argv);

    std::string getInput() const;
    std::string getOutput() const;
    std::string getConfigPath() const;
    bool isConfigDefault() const;
    Loggable::Severity getLogLevel() const;
    Type getType() const;
    Action getAction() const;

    bool readConfigFile();
    void readConfigLine(const std::string& line);
    std::string defaultConfig() const;

    static Action stringToAction(const std::string& source);
    static Action stringToAction(const std::string_view& source);
    static Type stringToType(const std::string& source);
    static Option stringToOption(const std::string& source);

private:
    void parseArguments();

    static std::string_view stripFlags(const std::string_view& option);
    static void strip(std::string& line);
    static void stripComment(std::string& line);

private:
    std::vector<std::string_view> arguments;
    std::optional<Action> action;
    std::optional<Type> outputType;
    std::optional<std::string> input;
    std::optional<std::string> output;
    std::optional<Loggable::Severity> logLevel;
    std::optional<std::string> configPath;
};
