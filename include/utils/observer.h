#ifndef PIORUN_UTILS_OBSERVER_H_
#define PIORUN_UTILS_OBSERVER_H_

#include <utility>

#include "time_fwd.h"

namespace pio {

template <typename Data>
struct ObserverWithDeadline {
  ObserverWithDeadline(Data &data, TimePoint deadline = NO_DEADLINE)
      : ptr_(&data), deadline_(deadline) {}
  ObserverWithDeadline(const ObserverWithDeadline &) = default;
  ObserverWithDeadline(ObserverWithDeadline &&o)
      : ptr_(std::exchange(o.ptr_, nullptr)),
        deadline_(std::exchange(o.deadline_, NO_DEADLINE)) {}
  ~ObserverWithDeadline() = default;
  ObserverWithDeadline &operator=(const ObserverWithDeadline &) = default;
  ObserverWithDeadline &operator=(ObserverWithDeadline &&o) {
    ptr_ = std::exchange(o.ptr_, nullptr);
    deadline_ = std::exchange(o.deadline_, NO_DEADLINE);
  }

  Data &operator*() { return *ptr_; }
  const Data &operator*() const { return *ptr_; }
  Data *operator->() { return ptr_; }
  const Data *operator->() const { return ptr_; }
  TimePoint deadline() const { return deadline_; }

 private:
  Data *ptr_;
  TimePoint deadline_;
};

}  // namespace pio

#endif  //  PIORUN_UTILS_OBSERVER_H_