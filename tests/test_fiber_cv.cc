#include <iostream>

#include "fiber/fiber.h"

using namespace std;
using namespace pio;

fiber::mutex mtx;
fiber::condition_variable cv;
bool isReady = false;

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

void worker() {
  LogInfo("Worker thread started.");
  this_fiber::sleep_for(std::chrono::seconds(2));
  std::lock_guard<fiber::mutex> lock(mtx);
  isReady = true;
  cv.notify_one();
}

void waiter() {
  LogInfo("Waiting for Worker finished.");
  std::unique_lock<fiber::mutex> lock(mtx);
  cv.wait(lock, [] { return isReady; });
  LogInfo("Worker coroutine finished.");
}

int main(int argc, char *argv[]) {
  go waiter;
  go worker;
  return 0;
}