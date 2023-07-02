add_executable(test_fiber test_fiber.cc)
target_link_libraries(test_fiber piorun)

add_executable(test_fiber_echoserver test_fiber_echoserver.cc)
target_link_libraries(test_fiber_echoserver piorun)

add_executable(test_fiber_cond test_fiber_cond.cc)
target_link_libraries(test_fiber_cond piorun)

add_executable(test_fiber_sem test_fiber_sem.cc)
target_link_libraries(test_fiber_sem piorun)

add_executable(test_fiber_rw test_fiber_rw.cc)
target_link_libraries(test_fiber_rw piorun)

add_executable(test_imsystem test_imsystem.cc)
target_link_libraries(test_imsystem piorun)