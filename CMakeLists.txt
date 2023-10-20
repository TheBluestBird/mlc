cmake_minimum_required(VERSION 3.5)
project(
    mlc
    VERSION 1.3.3
    DESCRIPTION "Media Library Compiler: rips your media library to a lossy compilation"
    LANGUAGES CXX
)
cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0079 NEW)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  list(APPEND COMPILE_OPTIONS -O3)
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
  list(APPEND COMPILE_OPTIONS -g)
  list(APPEND COMPILE_OPTIONS -Wall)
  list(APPEND COMPILE_OPTIONS -Wextra)
endif()

message("Compilation options: " ${COMPILE_OPTIONS})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(FLAC REQUIRED)
find_package(JPEG REQUIRED)
find_package(LAME REQUIRED)
find_package(TAGLIB REQUIRED)

add_executable(${PROJECT_NAME})
target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})

add_subdirectory(src)

target_link_libraries(${PROJECT_NAME}
    FLAC::FLAC
    LAME::LAME
    JPEG::JPEG
    TAGLIB::TAGLIB
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
