#include <iostream>
#include <thread>

#include "core/log.h"

void basic_test() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  logger->Info("This is a test for %s", "info");
  logger->Warning("This is a test for %s", "warning");
  logger->Error("This is a test %s", "error");
  logger->Fatal("This is a test %s", "fatal");
}

void test_in_thread() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  auto func = [logger]() {
    logger->Info("%s", "This is a test for thread");
  };

  std::thread t1(func);
  std::thread t2(func);

  t1.join();
  t2.join();
}

int main() { test_in_thread(); }