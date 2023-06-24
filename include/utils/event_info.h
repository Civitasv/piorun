#ifndef PIORUN_UTILS_EVENT_INFO_H_
#define PIORUN_UTILS_EVENT_INFO_H_
namespace pio {
enum class EventType : int { NONE, WAKEUP, TIMEOUT, ERROR, HANGUP };

using EventID = int;

enum class EventCategory : int { NONE, EPOLL, IOURING };
}  // namespace pio

#endif  // PIORUN_UTILS_EVENT_INFO_H_