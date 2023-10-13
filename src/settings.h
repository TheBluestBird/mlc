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
#include <regex>

#include "logger/logger.h"

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

    Settings(int argc, char **argv);

    std::string getInput() const;
    std::string getOutput() const;
    std::string getConfigPath() const;
    bool isConfigDefault() const;
    Logger::Severity getLogLevel() const;
    Type getType() const;
    Action getAction() const;
    unsigned int getThreads() const;
    bool matchNonMusic(const std::string& fileName) const;

    bool readConfigFile();
    void readConfigLine(const std::string& line);
    std::string defaultConfig() const;

    static Action stringToAction(const std::string& source);
    static Action stringToAction(const std::string_view& source);
    static Type stringToType(const std::string& source);

private:
    void parseArguments();

    static void strip(std::string& line);
    static void stripComment(std::string& line);
    static std::string resolvePath(const std::string& line);

private:
    std::vector<std::string_view> arguments;
    std::optional<Action> action;
    std::optional<Type> outputType;
    std::optional<std::string> input;
    std::optional<std::string> output;
    std::optional<Logger::Severity> logLevel;
    std::optional<std::string> configPath;
    std::optional<unsigned int> threads;
    std::optional<std::regex> nonMusic;
};
