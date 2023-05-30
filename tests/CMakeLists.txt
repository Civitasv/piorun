add_executable(test test.cc)
target_link_libraries(test piorun)

add_executable(test_log test_log.cc)
target_link_libraries(test_log piorun)
add_executable(test_config test_config.cc)
target_link_libraries(test_config piorun yaml-cpp)
