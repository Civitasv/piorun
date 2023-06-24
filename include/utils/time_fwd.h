#ifndef PIORUN_COROUTINE_TIME_H_
#define PIORUN_COROUTINE_TIME_H_

#include <chrono>

namespace pio {

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Duration = std::chrono::duration<int64_t, std::micro>;
inline constexpr const auto NO_DEADLINE = TimePoint::max();

}  // namespace pio

#endif  // PIORUN_COROUTINE_TIME_H_