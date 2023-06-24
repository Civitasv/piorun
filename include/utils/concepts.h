#ifndef PIORUN_COROUTINE_CONCEPTS_H_
#define PIORUN_COROUTINE_CONCEPTS_H_

#include <concepts>
#include <coroutine>

namespace pio {

template <typename T>
concept CoroutineHandle = std::convertible_to<T, std::coroutine_handle<>>;

template <typename T>
concept HandleWithContinuation =
    CoroutineHandle<T> && requires(T x) {
                            {
                              x.promise().continuation()
                              } -> std::convertible_to<std::coroutine_handle<>>;
                          };

template <typename T>
concept CoroutineWithContinuation = requires(T x) {
                                      { x.coro_handle() } -> HandleWithContinuation<>;
                                    };

}  // namespace pio

#endif  // PIORUN_COROUTINE_CONCEPTS_H_