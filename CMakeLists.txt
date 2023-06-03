cmake_minimum_required(VERSION 3.20)
project(piorun)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# output directory settings
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}
)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}
)
# ---------------------------

include(FetchContent)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://toscode.gitee.com/mirrors/yaml-cpp.git
    GIT_TAG yaml-cpp-0.7.0
)
FetchContent_MakeAvailable(yaml-cpp)
set(YAML_CPP_BUILD_TESTS OFF)

add_subdirectory(vendor)
add_subdirectory(src)
add_subdirectory(tests)
