#include "settings.h"

constexpr std::array<std::string_view, Settings::_actionsSize> actions({
    "convert",
    "help",
    "printConfig"
});

Settings::Settings(int argc, char ** argv):
    arguments(),
    action(std::nullopt),
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
            std::string_view act = stripFlags(arg);
            int dist = std::distance(actions.begin(), std::find(actions.begin(), actions.end(), act));
            if (dist < _actionsSize) {
                action = static_cast<Settings::Action>(dist);
                continue;
            }
        }

        if (getAction() == convert) {
            if (!input.has_value()) {
                input = arg;
                continue;
            }

            if (!output.has_value()) {
                input = arg;
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

Loggable::Severity Settings::getLogLevel() const {
    if (logLevel.has_value())
        return logLevel.value();
    else
        return Loggable::info;
}

std::string_view Settings::stripFlags(const std::string_view& option) {
    if (option[0] == '-') {
        if (option[1] == '-')
            return option.substr(2);

        return option.substr(1);
    }
    return option;
}
