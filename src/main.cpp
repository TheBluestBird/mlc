#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <memory>
#include <unistd.h>

#include "FLAC/stream_decoder.h"
#include <lame/lame.h>

#include "help.h"
#include "collection.h"
#include "taskmanager.h"
#include "settings.h"
#include "logger/logger.h"

int main(int argc, char **argv) {
    std::shared_ptr<Printer> logger = std::make_shared<Printer>();
    std::shared_ptr<Settings> settings = std::make_shared<Settings>(argc, argv);

    switch (settings->getAction()) {
        case Settings::help:
            printHelp();
            return 0;
        case Settings::config:
            std::cout << settings->defaultConfig() << std::endl;
            return 0;
        case Settings::convert:
            std::cout << "Converting..." << std::endl;
            break;
        default:
            std::cout << "Error in action" << std::endl;
            return -1;
    }

    if (!settings->readConfigFile() && settings->isConfigDefault()) {
        std::string defaultConfigPath = settings->getConfigPath();
        std::ofstream file(defaultConfigPath, std::ios::out | std::ios::trunc);
        if (file.is_open()) {
            std::cout << "Writing default config to " << defaultConfigPath << std::endl;
            file << settings->defaultConfig();
        } else {
            std::cout << "Couldn't open " << defaultConfigPath << " to write default config" << std::endl;
        }
    }

    std::string input = settings->getInput();
    if (input.empty()) {
        std::cout << "Input folder is not specified, quitting" << std::endl;
        return -2;
    }

    std::string output = settings->getOutput();
    if (output.empty()) {
        std::cout << "Output folder is not specified, quitting" << std::endl;
        return -3;
    }

    logger->setSeverity(settings->getLogLevel());
    TaskManager taskManager(settings, logger);
    taskManager.start();

    std::chrono::time_point start = std::chrono::system_clock::now();
    Collection collection(input, &taskManager);
    collection.convert(output);

    taskManager.wait();
    std::cout << std::endl;
    taskManager.stop();

    std::chrono::time_point end = std::chrono::system_clock::now();
    std::chrono::duration<double> seconds = end - start;
    std::cout  << "Encoding is done, it took " << seconds.count() << " seconds in total, enjoy!" << std::endl;

    return 0;
}
