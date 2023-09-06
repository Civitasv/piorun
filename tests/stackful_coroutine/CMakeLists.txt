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

add_executable(test_fiber_http_server test_fiber_http_server.cc)
target_link_libraries(test_fiber_http_server piorun)

# 添加自定义命令来复制文件夹
add_custom_command(TARGET test_fiber_http_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:test_fiber_http_server>/resources
)
