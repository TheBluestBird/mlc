#include "help.h"

#include "iostream"

static const char* help =
#include "generated/help"
;

void printHelp() {
    std::cout << help << std::endl;
}
