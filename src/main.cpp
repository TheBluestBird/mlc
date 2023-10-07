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

int main(int argc, char **argv) {
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

    TaskManager taskManager(settings);
    taskManager.start();

    std::chrono::time_point start = std::chrono::system_clock::now();
    Collection collection(settings->getInput(), &taskManager);
    collection.convert(settings->getOutput());

    taskManager.printProgress();
    taskManager.wait();
    std::chrono::time_point end = std::chrono::system_clock::now();
    std::chrono::duration<double> seconds = end - start;
    std::cout << std::endl << "Encoding is done, it took " << seconds.count() << " seconds in total, enjoy!" << std::endl;

    taskManager.stop();

    return 0;
}
