#include "settings.h"

static const char* defaultConfig =
#include "generated/default.conf"
;

enum class Flag {
    config,
    help,
    none
};

enum class Option {
    level,
    type,
    source,
    destination,
    parallel,
    filesToCopy,
    exclude,
    encodingQuality,
    outputQuality,
    vbr,
    _optionsSize
};

using Literals = std::array<std::string_view, 2>;
constexpr std::array<Literals, static_cast<int>(Flag::none)> flags({{
    {"-c", "--config"},
    {"-h", "--help"}
}});

constexpr std::array<std::string_view, Settings::_actionsSize> actions({
    "convert",
    "help",
    "config"
});

constexpr std::array<std::string_view, static_cast<int>(Option::_optionsSize)> options({
    "level",
    "type",
    "source",
    "destination",
    "parallel",
    "filesToCopy",
    "exclude",
    "encodingQuality",
    "outputQuality",
    "vbr"
});

constexpr std::array<std::string_view, Settings::_typesSize> types({
    "mp3"
});

constexpr unsigned int maxQuality = 9;
constexpr unsigned int minQuality = 0;

bool is_space(char ch){
    return std::isspace(static_cast<unsigned char>(ch));
}

Flag getFlag(const std::string_view arg) {
    for (unsigned char i = 0; i < flags.size(); ++i) {
        const Literals& lit = flags[i];
        unsigned char dist = std::distance(lit.begin(), std::find(lit.begin(), lit.end(), arg));
        if (dist < lit.size())
            return static_cast<Flag>(i);
    }

    return Flag::none;
}

Option stringToOption(const std::string& source) {
    unsigned char dist = std::distance(options.begin(), std::find(options.begin(), options.end(), source));
    if (dist < static_cast<unsigned char>(Option::_optionsSize))
        return static_cast<Option>(dist);

    return Option::_optionsSize;
}

Settings::Settings(int argc, char ** argv):
    arguments(),
    action(std::nullopt),
    outputType(std::nullopt),
    input(std::nullopt),
    output(std::nullopt),
    logLevel(std::nullopt),
    configPath(std::nullopt),
    threads(std::nullopt),
    nonMusic(std::nullopt),
    encodingQuality(std::nullopt),
    outputQuality(std::nullopt),
    vbr(std::nullopt)
{
    for (int i = 1; i < argc; ++i)
        arguments.push_back(argv[i]);

    parseArguments();
}

void Settings::parseArguments() {
    Flag flag = Flag::none;
    for (unsigned int i = 0; i < arguments.size(); ++i) {
        const std::string_view& arg = arguments[i];
        if (i == 0) {
            Action act = stringToAction(arg);
            if (act < _actionsSize) {
                action = act;
                continue;
            }
        }

        switch (flag) {
            case Flag::config:
                configPath = arg;
                flag = Flag::none;
                continue;
            case Flag::none:
                flag = getFlag(arg);
                break;
            default:
                break;
        }

        switch (flag) {
            case Flag::none:
                break;
            case Flag::help:
                action = help;
                flag = Flag::none;
                continue;
            default:
                continue;
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
        return resolvePath(input.value());
    else
        return "";
}

std::string Settings::getOutput() const {
    if (output.has_value())
        return resolvePath(output.value());
    else
        return "";
}

std::string Settings::getConfigPath() const {
    if (configPath.has_value())
        return resolvePath(configPath.value());
    else
        return resolvePath("~/.config/mlc.conf");
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

unsigned int Settings::getThreads() const {
    if (threads.has_value())
        return threads.value();
    else
        return 0;
}

unsigned char Settings::getOutputQuality() const {
    if (outputQuality.has_value())
        return outputQuality.value();
    else
        return minQuality;      //it means max possible quality, min is for min enum value
}

unsigned char Settings::getEncodingQuality() const {
    if (encodingQuality.has_value())
        return encodingQuality.value();
    else
        return minQuality;      //it means max possible quality, min is for min enum value
}

bool Settings::getVBR() const {
    if (vbr.has_value())
        return vbr.value();
    else
        return true;
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
    if (option == Option::_optionsSize)
        return;

    std::string value;
    std::getline(stream >> std::ws, value);
    strip(value);
    if (value.empty())
        return;

    switch (option) {
        case Option::level: {
            std::string lv;
            if (!logLevel.has_value() && std::istringstream(value) >> lv) {
                Logger::Severity level = Logger::stringToSeverity(lv);
                if (level < Logger::Severity::_severitySize)
                    logLevel = level;
            }
        }   break;
        case Option::type: {
            std::string tp;
            if (!outputType.has_value() && std::istringstream(value) >> tp) {
                Type type = stringToType(tp);
                if (type < _typesSize)
                    outputType = type;
            }
        }   break;
        case Option::source: {
            if (!input.has_value()) {
                input = value;
            } else if (!output.has_value()) {
                output = input;
                input = value;
            }
        }   break;
        case Option::destination: {
            if (!output.has_value())
                output = value;
        }   break;
        case Option::parallel: {
            unsigned int count;
            if (!threads.has_value() && std::istringstream(value) >> count)
                threads = count;
        }   break;
        case Option::filesToCopy: {
            if (!nonMusic.has_value()) {
                if (value == "all")
                    value = "";
                else if (value == "none")
                    value = "a^";

                nonMusic = value;
            }
        }   break;
        case Option::exclude: {
            if (!excluded.has_value()) 
                excluded = value;
        }   break;
        case Option::outputQuality: {
            unsigned int quality;
            if (!outputQuality.has_value() && std::istringstream(value) >> quality)
                outputQuality = std::clamp(quality, minQuality, maxQuality);
        }   break;
        case Option::encodingQuality: {
            unsigned int quality;
            if (!encodingQuality.has_value() && std::istringstream(value) >> quality)
                encodingQuality = std::clamp(quality, minQuality, maxQuality);
        }   break;
        case Option::vbr: {
            bool vb;
            if (!vbr.has_value() && std::istringstream(value) >> std::boolalpha >> vb)
                vbr = vb;
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

Settings::Type Settings::stringToType(const std::string& source) {
    unsigned char dist = std::distance(types.begin(), std::find(types.begin(), types.end(), source));
    if (dist < _typesSize)
        return static_cast<Type>(dist);

    return _typesSize;
}

std::string Settings::resolvePath(const std::string& line) {
    if (line.size() > 0 && line[0] == '~')
        return getenv("HOME") + line.substr(1);
    else
        return line;
}

bool Settings::matchNonMusic(const std::string& fileName) const {
    if (!nonMusic.has_value())
        return true;

    return std::regex_search(fileName, nonMusic.value());
}

bool Settings::isExcluded(const std::string& path) const {
    if (!excluded.has_value())
        return false;

    return std::regex_search(path, excluded.value());
}

