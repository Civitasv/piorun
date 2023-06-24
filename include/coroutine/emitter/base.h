#ifndef PIORUN_COROUTINE_EMITTER_BASE_H_
#define PIORUN_COROUTINE_EMITTER_BASE_H_

#include "coroutine/awaitable/data.h"

namespace pio {
namespace emitter {

/**
 * @brief Emitters are external sources of events
 *        (connection arrived, socket ready for writing,
 *         condition evaluated to true, timeout expired, etc.)
 */
struct Base {
  virtual awaitable::Data *Emit() = 0;
  // Register the event to be wathced.
  virtual void NotifyArrival(awaitable::Data *) = 0;
  // Unregister the event.
  virtual void NotifyDeparture(awaitable::Data *) = 0;
  virtual bool IsEmpty() = 0;

  virtual ~Base() {};
};

}  // namespace emitter

}  // namespace pio

#endif  // PIORUN_COROUTINE_EMITTER_BASE_H_