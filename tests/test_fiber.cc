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
    this_fiber::yield();
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

void printInfo() {
  time_t now = time(NULL);
  tm t;
  localtime_r(&now, &t);
  // auto id = std::this_thread::get_id();
  auto id = this_fiber::get_thread_id();
  printf(
    "[%02d:%02d:%02d]: thread: %d fiber: %llu\n", t.tm_hour, t.tm_min, t.tm_sec, id, this_fiber::get_id()
  );
}

void sleepCo(int x) {
  this_fiber::sleep_for(seconds{x});
  printInfo();
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

  // 2. test scheduler.
  for (int i = 0; i < 10; i++) {
    go std::bind(sleepCo, i + 1);
  }

  // 3. test recursive.
  go [] {
    printInfo();
    go [] {
      printInfo();
      go [] {
        this_fiber::sleep_for(15s);
        printInfo();
      };
    };
  };

  return 0;

}
