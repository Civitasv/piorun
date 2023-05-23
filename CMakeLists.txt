cmake_minimum_required(VERSION 3.20)
project(piorun)

set(CMAKE_C_STANDARD 20)

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

add_subdirectory(vendor)
add_subdirectory(src)
add_subdirectory(tests)
