add_executable(run run.cc)

add_subdirectory(piorun)

target_link_libraries(run PUBLIC piorun)