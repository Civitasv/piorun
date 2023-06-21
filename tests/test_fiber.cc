#include <cstdio>
#include <chrono>
#include <unistd.h>
#include <iostream>

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
  
  Generator(void*(*pfn)(void*)) : value() {
    this->co = new StCoRoutine(pfn, this);
    this->co->Resume();
  }

 ~Generator() { delete this->co; }

  iterator begin() { return {this}; }
  iterator_end_sentinel end() { return {}; }

  void yield(const T& x) {
    value = x;
    this_coroutine::yield();
  }

  T value;
  StCoRoutine* co;

};

void* fibonacci(void* arg) {
  Generator<int>* g = (Generator<int>*)(arg);
  int a = 0, b = 1;
  for (int i = 0; i < 10; i++) {
    g->yield(a);
    b = a + b;
    a = b - a;
  }
  return 0;
}

void* sleepCo(void*) {
  while (true) {
    printf("current fiber: %llx\n", this_coroutine::get_id());
    this_coroutine::sleep_for(1s);
  }
  return 0;
}


int main(int argc, const char* agrv[]) {

  for (auto&& x : Generator<int>{fibonacci}) {
    std::cout << x << std::endl;
  }

  std::vector<pio::fiber::StCoRoutine> fibers;
  fibers.reserve(5);

  for (int i = 0; i < 5; i++) {
    fibers.emplace_back(sleepCo, nullptr);
  }

  for (auto&& fiber : fibers) {
    fiber.Resume();
  }

  co_eventloop(nullptr, nullptr);

  return 0;

}
