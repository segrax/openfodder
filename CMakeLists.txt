cmake_minimum_required(VERSION 3.10.0)

if (POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif ()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, default to Release.")
    set(CMAKE_BUILD_TYPE "Release")
endif()

project(openfodder VERSION 1.5.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${openfodder_SOURCE_DIR}/cmake)

find_package(SDL2 REQUIRED)
find_package(SDL2Mixer REQUIRED)

# Locate git binary to provide information to the build environment
find_package(Git)

if(GIT_FOUND)
# Define short commit hash.
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE OPENFODDER_COMMIT_SHA1_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
# Fallback in case the git exe can't be found.
    set(OPENFODDER_COMMIT_SHA1_SHORT "deadbeef")
endif()

configure_file(cmake/gitver.hpp.in Source/gitver.hpp @ONLY)

file(GLOB_RECURSE GAMEENGINE_HEADERS "Source/*.hpp")
file(GLOB_RECURSE GAMEENGINE_SOURCE "Source/*.cpp")

add_executable(openfodder ${GAMEENGINE_SOURCE} ${GAMEENGINE_HEADERS})

target_include_directories(openfodder PRIVATE
    Source
    ${CMAKE_CURRENT_BINARY_DIR}/Source)

target_link_libraries(openfodder SDL2::SDL2 SDL2_mixer::SDL2_mixer)
