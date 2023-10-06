set(SOURCES
    main.cpp
    help.cpp
    decoded.cpp
    flactomp3.cpp
    collection.cpp
    taskmanager.cpp
    loggable.cpp
    settings.cpp
)

set(HEADERS
    help.h
    decoded.h
    flactomp3.h
    collection.h
    taskmanager.h
    loggable.h
    settings.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
