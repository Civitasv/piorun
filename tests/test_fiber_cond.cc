#include <queue>
#include <iostream>
#include "fiber/fiber.h"

using namespace std;
using namespace pio;
using namespace pio::fiber;
using namespace pio::this_fiber;

struct stTask_t { int id; };

pio::fiber::mutex mtx;
pio::fiber::semaphore full(0);
pio::fiber::semaphore space(5);
std::queue<stTask_t*> tasks;

void Producer(int x) {
  int id = x;
  for (int i = 0; i < 100; i++) {
    // 生产一个产品
    stTask_t* task = new stTask_t;
    task->id = id++;
    printf("%d: %llx produce task %5d\n", get_thread_id(), get_id(), task->id);

    space.wait();
    mtx.lock();
    
    // 把产品放入缓冲区
    tasks.push(task);
    
    mtx.unlock();
    full.signal();

    sleep_for(std::chrono::milliseconds(random() % 50 + 1));
  }
}

void Consumer() {
  while (true) {
    full.wait();
    mtx.lock();

    // 从缓冲区取出一个产品
    stTask_t* task = tasks.front();
    tasks.pop();

    mtx.unlock();
    space.signal();

    printf("%d: %llx consume task %5d\n", get_thread_id(), get_id(), task->id);
    delete task;

    sleep_for(std::chrono::milliseconds(random() % 50 + 1));
  }
}

int main(int argc, char *argv[]) {
  for (int i = 0; i < 2; i++) {
    go Consumer;
  }
  for (int i = 0; i < 5; i++) {
    go std::bind(Producer, i * 1000);
  }
  return 0;
}