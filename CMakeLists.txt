cmake_minimum_required(VERSION 3.0)
project(
    mlc
    VERSION 1.0.1
    DESCRIPTION "Media Library Compiler: rips your media library to a lossy compilation"
    LANGUAGES CXX
)
cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0079 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(LAME REQUIRED)
find_package(FLAC REQUIRED)
find_package(JPEG REQUIRED)

add_executable(mlc
    main.cpp
    help.cpp
    decoded.cpp
    flactomp3.cpp
    collection.cpp
    taskmanager.cpp
)

target_link_libraries(mlc
    ${LAME_LIBRARIES}
    FLAC
    JPEG::JPEG
)

install(TARGETS mlc RUNTIME DESTINATION bin)
