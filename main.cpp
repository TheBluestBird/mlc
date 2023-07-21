#include <iostream>
#include <string>
#include <chrono>
#include <unistd.h>

#include "FLAC/stream_decoder.h"
#include <lame/lame.h>

#include "help.h"
#include "collection.h"
#include "taskmanager.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "Insufficient amount of arguments, launch with \"--help\" argument to see usage" << std::endl;
        return 1;
    }

    const std::string firstArgument(argv[1]);
    const std::string secondArgument(argv[2]);

    if (firstArgument == "--help") {
        printHelp();
        return 0;
    }

    TaskManager taskManager;
    taskManager.start();

    std::chrono::time_point start = std::chrono::system_clock::now();
    Collection collection(firstArgument, &taskManager);
    collection.convert(secondArgument);

    taskManager.printProgress();
    taskManager.wait();
    std::chrono::time_point end = std::chrono::system_clock::now();
    std::chrono::duration<double> seconds = end - start;
    std::cout << std::endl << "took " << seconds.count() << std::endl;

    taskManager.stop();

    return 0;
}
