function(make_includable input_file output_file)
    file(READ ${input_file} content)
    set(delim "for_c++_include")
    set(content "R\"${delim}(\n${content})${delim}\"")
    file(WRITE ${output_file} "${content}")
endfunction(make_includable)

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

make_includable(default.conf ${CMAKE_BINARY_DIR}/generated/default.conf)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR})
