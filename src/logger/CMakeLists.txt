set(SOURCES
    logger.cpp
    printer.cpp
    accumulator.cpp
)

set(HEADERS
    logger.h
    printer.h
    accumulator.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
