#include <iostream>
#include <string>

#include "FLAC/stream_decoder.h"
#include <lame/lame.h>

#include "help.h"
#include "flactomp3.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Insufficient amount of arguments, launch with \"--help\" argument to see usage" << std::endl;
        return 1;
    }

    const std::string firstArgument(argv[1]);

    if (firstArgument == "--help") {
        printHelp();
        return 0;
    }

    FLACtoMP3 pipe;
    pipe.setInputFile(firstArgument);
    pipe.setOutputFile("out.mp3");
    pipe.run();

    return 0;
}
