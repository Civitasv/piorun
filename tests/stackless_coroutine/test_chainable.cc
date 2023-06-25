#include <iostream>

#include "coroutine/awaitable/event.h"
#include "coroutine/task/chainable.h"
#include "coroutine/task/synchronous.h"

using namespace pio;

task::Chainable AsyncOP(bool flag) {
  if (flag) {
    co_return awaitable::Result{
        EventType::ERROR, 1,
        "Couldn't find the truth in the eye of the beholder."};
  } else {
    co_return awaitable::Result{EventType::WAKEUP, 0, ""};
  }
}

task::Synchronous Demo() {
  if (auto ret = co_await AsyncOP(true); !ret) {
    std::cerr << "Asynchronous operation failed: " << ret.err_message << "\n";
  }
  if (auto ret = co_await AsyncOP(false); !ret) {
    std::cerr << "Unexpected failure: " << ret.err_message << "\n";
  }
  co_return;
}

int main() { Demo(); }