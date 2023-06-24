target_sources(
    piorun
    PUBLIC
    scheduler.cc
)

add_subdirectory(async)
add_subdirectory(awaitable)
add_subdirectory(emitter)