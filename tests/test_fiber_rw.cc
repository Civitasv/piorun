#include <queue>
#include <iostream>
#include "fiber/fiber.h"

using namespace pio;
using namespace fiber;
using namespace this_fiber;

int mydata = 0;

shared_mutex rwmtx;

void Reader(int id) {
  for (int i = 0; i < 100; i++) {
    rwmtx.lock_shared();
    printf("[%d:%llx] Reader %4d reads data: %4d\n", get_thread_id(), get_id(), id, mydata);
    rwmtx.unlock_shared();
    sleep_for(std::chrono::milliseconds(random() % 50 + 1));
  }
  
}

void Writer(int id) {
  for (int i = 0; i < 10; i++) {
    rwmtx.lock();
    mydata = random() % 100;
    printf("[%d:%llx] Writer %4d write data: %4d\n", get_thread_id(), get_id(), id, mydata);
    rwmtx.unlock();
    sleep_for(std::chrono::milliseconds(random() % 100 + 20));
  }
}

int main(int argc, char *argv[]) {

  for (int i = 0; i < 1000; i++) {
    go std::bind(Reader, i);
    sleep_for(std::chrono::milliseconds(random() % 50 + 1));
    go std::bind(Writer, i);
    sleep_for(std::chrono::milliseconds(random() % 50 + 1));
  }
  return 0;

}