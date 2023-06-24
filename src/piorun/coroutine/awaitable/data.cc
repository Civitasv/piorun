#include "coroutine/awaitable/data.h"

namespace pio {
namespace awaitable {
std::ostream &operator<<(std::ostream &s, const Result &res) {
  const char *types[] = {"None", "WakeUp", "Timeout", "Error"};
  s << "EventType: " << types[(int)res.result_type] << " Error: " << res.err
    << " Error Message: " << res.err_message;
  return s;
}
}  // namespace awaitable
}  // namespace pio