add_library(piorun socket.cc piorun.cc)

target_include_directories(piorun PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(piorun PUBLIC rt pthread) # for sem and pthread
target_link_libraries(piorun PUBLIC yaml-cpp)

add_subdirectory(core)
add_subdirectory(coroutine)
add_subdirectory(epoll)