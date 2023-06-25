#ifndef PIORUN_COROUTINE_EMITTER_BASE_H_
#define PIORUN_COROUTINE_EMITTER_BASE_H_

#include "coroutine/awaitable/event.h"

namespace pio {
namespace emitter {

/**
 * @brief Emitters are external sources of events
 *        (connection arrived, socket ready for writing,
 *         condition evaluated to true, timeout expired, etc.)
 */
struct Base {
  virtual awaitable::Event *Emit() = 0;  ///< 处理一个事件
  // Register the event to be watched.
  virtual void NotifyArrival(awaitable::Event *) = 0;
  // Unregister the event.
  virtual void NotifyDeparture(awaitable::Event *) = 0;
  virtual bool IsEmpty() = 0;

  virtual ~Base(){};
};

}  // namespace emitter

}  // namespace pio

#endif  // PIORUN_COROUTINE_EMITTER_BASE_H_