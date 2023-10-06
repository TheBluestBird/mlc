#include <iostream>
#include <string>
#include <chrono>
#include <unistd.h>

#include "FLAC/stream_decoder.h"
#include <lame/lame.h>

#include "help.h"
#include "collection.h"
#include "taskmanager.h"
#include "settings.h"

int main(int argc, char **argv) {
    Settings settings(argc, argv);

    switch (settings.getAction()) {
        case Settings::help:
            printHelp();
            return 0;
        case Settings::printConfig:
            //printHelp();
            return 0;
        case Settings::convert:
            std::cout << "Converting..." << std::endl;
            break;
        default:
            std::cout << "Error in action" << std::endl;
            return -1;
    }

    TaskManager taskManager;
    taskManager.start();

    std::chrono::time_point start = std::chrono::system_clock::now();
    Collection collection(settings.getInput(), &taskManager);
    collection.convert(settings.getOutput());

    taskManager.printProgress();
    taskManager.wait();
    std::chrono::time_point end = std::chrono::system_clock::now();
    std::chrono::duration<double> seconds = end - start;
    std::cout << std::endl << "Encoding is done, it took " << seconds.count() << " seconds in total, enjoy!" << std::endl;

    taskManager.stop();

    return 0;
}
