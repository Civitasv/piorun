#include "core/mutex.h"
#include <stdexcept>

namespace pio {

Semaphore::Semaphore(u32 count) {
  if (sem_init(&semaphore_, 0, count)) {
    throw std::logic_error("sem_init error");
  }
}

Semaphore::~Semaphore() 
{ sem_destroy(&semaphore_); }

void Semaphore::Wait() {
  if (sem_wait(&semaphore_)) {
    throw std::logic_error("sem_wait error");
  }
}

void Semaphore::Signal() {
  if (sem_post(&semaphore_)) {
    throw std::logic_error("sem_post error");
  }
}

}