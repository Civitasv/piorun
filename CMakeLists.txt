cmake_minimum_required(VERSION 3.20)
project(piorun)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# output directory settings
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/${CMAKE_HOST_SYSTEM_NAME}/${PROJECT_NAME}/lib
)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/${CMAKE_HOST_SYSTEM_NAME}/${PROJECT_NAME}/lib
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/${CMAKE_HOST_SYSTEM_NAME}/${PROJECT_NAME}/lib
)
# ---------------------------

find_package(yaml-cpp REQUIRED)
if(yaml-cpp_FOUND)
    include_directories(${YAML_CPP_INCLUDE_DIRS})
endif()

add_subdirectory(vendor)
add_subdirectory(src)
add_subdirectory(tests)
