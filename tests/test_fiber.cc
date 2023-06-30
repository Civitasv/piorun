#include <cstdio>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <signal.h>

#include "fiber/fiber.h"

using namespace pio;
using namespace pio::fiber;
using namespace std::chrono;

template <typename T>
struct Generator {

  struct iterator_end_sentinel {};

  struct iterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = T;
    
    iterator(Generator* g) : generator(g) {}

    T &operator*() { return generator->value; }

    void operator++() { generator->co->Resume(); }
    
    bool operator!=(iterator_end_sentinel) { return !generator->co->Done(); }

    Generator* generator;
  };
  
  Generator(std::function<void(Generator&)> pfn) : value() {
    auto cb = std::bind(pfn, std::ref(*this));
    this->co = new Fiber(cb);
    this->co->Resume();
  }

 ~Generator() { delete this->co; }

  iterator begin() { return {this}; }
  iterator_end_sentinel end() { return {}; }

  void yield(const T& x) {
    value = x;
    this_fiber::co_self()->Yield();
  }

  T value;
  Fiber* co;

};

void fibonacci(Generator<int>& g) {
  int a = 0, b = 1;
  for (int i = 0; i < 10; i++) {
    g.yield(a);
    b = a + b;
    a = b - a;
  }
}

static void LogInfo(const std::string& msg) {
  struct tm t;
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t tsec = now.tv_sec;
  localtime_r(&tsec, &t);
  printf (
    "%d-%02d-%02d %02d:%02d:%02d.%03ld [info] : [%d:%p]: %s\n", 
    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec / 1000,
    this_fiber::get_thread_id(), this_fiber::co_self(), msg.c_str()
  );
}

void sleepCo(int x) {
  this_fiber::sleep_for(milliseconds{x});
  // printInfo();
}

int stop = 0;

void sighdr(int x) {
  stop = 1;
}

int main(int argc, const char* agrv[]) {
  
  // 1. test fiber switch.
  printf("test fib  : ");
  for (auto&& x : Generator<int>{fibonacci}) {
    printf("%d ", x);
  }
  printf("\n");

  // 2. test recursive.
  go [] {
    LogInfo("first");
    go [] {
      LogInfo("second");
      go [] {
        this_fiber::sleep_for(15s);
        LogInfo("third");
      };
    };
  };

  // 3. test yield
  go [] {
    printf("before yield\n");
    this_fiber::yield();
    printf("after yield\n");
  };

  // 4. test scheduler.
  // 不要在 go 外面大量高并发提交任务，此时会频繁占用锁，如果需要大量高并发提交任务，新建一个 go 然后在里面提交
  go [] {
    for (int i = 0; i < 1000; i++) {
      // simulate 100 requests per milliseconds.
      for (int j = 0; j < 100; j++) { 
        go std::bind(sleepCo, i * 20 + 1);
      }
      this_fiber::sleep_for(std::chrono::milliseconds(1));
    }
  };

  printf("finished test...\n");

  return 0;

}
