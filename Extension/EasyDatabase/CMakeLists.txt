cmake_minimum_required(VERSION 2.6)
project(easydatabase)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MACOSX_RPATH 1)

set(ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/output)
set(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/output)
set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/output)

include_directories("${PROJECT_SOURCE_DIR}/external/rapidxml-1.13/include")


file(GLOB sources 
    "${PROJECT_SOURCE_DIR}/inc/*.h"
    "${PROJECT_SOURCE_DIR}/inc/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.c"
)

find_package(Threads)
add_library(libeasydatabase SHARED ${sources})
target_link_libraries(libeasydatabase ${CMAKE_THREAD_LIBS_INIT} YAML_CPP)
target_include_directories(libeasydatabase PUBLIC ${PROJECT_SOURCE_DIR}/inc)
SET_TARGET_PROPERTIES(libeasydatabase PROPERTIES PREFIX "")