#include <iostream>

#include "core/thread.h"

void fn() {
  std::cout << "name: " << pio::Thread::GetCurrentThreadName()
            << " | this.name: " << pio::Thread::GetCurrentThread()->get_thread_name()
            << " | id: " << pio::Thread::GetCurrentThreadId()
            << " | this.id: " << pio::Thread::GetCurrentThread()->get_system_id()
            << std::endl;
  sleep(20);
}

int main(int argc, const char* argv[]) {

  std::cout << "thread test begin" << std::endl;
  std::vector<std::shared_ptr<pio::Thread>> threads;
  
  for (int i = 0; i < 5; i++) {
    std::shared_ptr thread = std::make_shared<pio::Thread>(&fn, "name_" + std::to_string(i));
    threads.push_back(thread);
  }

  for (int i = 0; i < 5; i++) {
    threads[i]->Join();
  }

  std::cout << "thread test finished" << std::endl;

}