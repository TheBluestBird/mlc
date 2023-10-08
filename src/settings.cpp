#include "settings.h"

static const char* defaultConfig =
#include "generated/default.conf"
;

constexpr std::array<std::string_view, Settings::_actionsSize> actions({
    "convert",
    "help",
    "config"
});

constexpr std::array<std::string_view, Settings::_optionsSize> options({
    "level",
    "type"
});

constexpr std::array<std::string_view, Settings::_typesSize> types({
    "mp3"
});

bool is_space(char ch){
    return std::isspace(static_cast<unsigned char>(ch));
}

Settings::Settings(int argc, char ** argv):
    arguments(),
    action(std::nullopt),
    outputType(std::nullopt),
    input(std::nullopt),
    output(std::nullopt),
    logLevel(std::nullopt)
{
    for (int i = 1; i < argc; ++i)
        arguments.push_back(argv[i]);

    parseArguments();
}

void Settings::parseArguments() {
    for (int i = 0; i < arguments.size(); ++i) {
        const std::string_view& arg = arguments[i];
        if (i == 0) {
            Action act = stringToAction(stripFlags(arg));
            if (act < _actionsSize) {
                action = act;
                continue;
            }
        }

        if (getAction() == convert) {
            if (!input.has_value()) {
                input = arg;
                continue;
            }

            if (!output.has_value()) {
                output = arg;
                continue;
            }
        }
    }
}

Settings::Action Settings::getAction() const {
    if (action.has_value())
        return action.value();
    else
        return convert;
}

Settings::Type Settings::getType() const {
    if (outputType.has_value())
        return outputType.value();
    else
        return mp3;
}

std::string Settings::getInput() const {
    if (input.has_value())
        return input.value();
    else
        return "";
}

std::string Settings::getOutput() const {
    if (output.has_value())
        return output.value();
    else
        return "";
}

std::string Settings::getConfigPath() const {
    if (configPath.has_value())
        return configPath.value();
    else
        return std::string(getenv("HOME")) + "/.config/mlc.conf";
}

bool Settings::isConfigDefault() const {
    return !configPath.has_value();
}

Logger::Severity Settings::getLogLevel() const {
    if (logLevel.has_value())
        return logLevel.value();
    else
        return Logger::Severity::info;
}

std::string_view Settings::stripFlags(const std::string_view& option) {
    if (option[0] == '-') {
        if (option[1] == '-')
            return option.substr(2);

        return option.substr(1);
    }
    return option;
}

void Settings::strip(std::string& line) {
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not_fn(is_space)));
    line.erase(std::find_if(line.rbegin(), line.rend(), std::not_fn(is_space)).base(), line.end());
}

void Settings::stripComment(std::string& line) {
    std::string::size_type index = line.find('#');
    if (index != std::string::npos)
        line.erase(index);

    strip(line);
}

std::string Settings::defaultConfig() const {
    return ::defaultConfig + 1;
}

bool Settings::readConfigFile() {
    std::ifstream file;
    file.open(getConfigPath(), std::ios::in);
    if (file.is_open()){
        std::string tp;
        while(getline(file, tp)) {
            stripComment(tp);
            if (!tp.empty())
                readConfigLine(tp);
        }
        file.close();
        return true;
    }

    return false;
}

void Settings::readConfigLine(const std::string& line) {
    std::istringstream stream(line);
    std::string key;
    if (!(stream >> key))
        return;

    Option option = stringToOption(key);
    if (option == _optionsSize)
        return;

    switch (option) {
        case level: {
            std::string lv;
            if (stream >> lv) {
                Logger::Severity level = Logger::stringToSeverity(lv);
                if (level < Logger::Severity::_sevetirySize)
                    logLevel = level;
            }
        }   break;
        case type: {
            std::string lv;
            if (stream >> lv) {
                Type type = stringToType(lv);
                if (type < _typesSize)
                    outputType = type;
            }
        }   break;
        default:
            break;
    }
}

Settings::Action Settings::stringToAction(const std::string& source) {
    unsigned char dist = std::distance(actions.begin(), std::find(actions.begin(), actions.end(), source));
    if (dist < _actionsSize)
        return static_cast<Action>(dist);

    return _actionsSize;
}

Settings::Action Settings::stringToAction(const std::string_view& source) {
    unsigned char dist = std::distance(actions.begin(), std::find(actions.begin(), actions.end(), source));
    if (dist < _actionsSize)
        return static_cast<Action>(dist);

    return _actionsSize;
}

Settings::Option Settings::stringToOption(const std::string& source) {
    unsigned char dist = std::distance(options.begin(), std::find(options.begin(), options.end(), source));
    if (dist < _optionsSize)
        return static_cast<Option>(dist);

    return _optionsSize;
}

Settings::Type Settings::stringToType(const std::string& source) {
    unsigned char dist = std::distance(types.begin(), std::find(types.begin(), types.end(), source));
    if (dist < _typesSize)
        return static_cast<Type>(dist);

    return _typesSize;
}
