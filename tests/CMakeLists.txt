add_executable(test test.cc)
target_link_libraries(test piorun)

add_executable(test_log test_log.cc)
target_link_libraries(test_log piorun)

add_executable(test_thread test_thread.cc)
target_link_libraries(test_thread piorun)

add_executable(test_fiber test_fiber.cc)
target_link_libraries(test_fiber piorun)

add_executable(test_fiber_echoserver test_fiber_echoserver.cc)
target_link_libraries(test_fiber_echoserver piorun)

add_executable(test_fiber_cond test_fiber_cond.cc)
target_link_libraries(test_fiber_cond piorun)

add_executable(test_config test_config.cc)
target_link_libraries(test_config piorun yaml-cpp)

add_subdirectory(stackless_coroutine)
