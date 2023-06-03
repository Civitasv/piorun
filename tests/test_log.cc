#include <iostream>
#include <thread>

#include "core/log.h"

void basic_test_with_format() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  logger->InfoF("This is a test for %s", "info");
  logger->WarningF("This is a test for %s", "warning");
  logger->ErrorF("This is a test for %s", "error");
  logger->FatalF("This is a test for %s", "fatal");
}

void basic_test_without_format() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  logger->Info("This is a test for ", "info");
  logger->Warning("This is a test for ", "warning");
  logger->Error("This is a test for ", "error");
  logger->Fatal("This is a test for ", "fatal");
}

void test_in_thread() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  auto func = [logger]() {
    logger->Info(std::this_thread::get_id());
    logger->InfoF("%s", "This is a test for thread");
  };

  std::thread t1(func);
  std::thread t2(func);

  t1.join();
  t2.join();
}

void test_stream_out() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  logger << "TEST "
         << "For "
         << "Stream "
         << "Out " << 1 << '\n';
}

int main() { test_stream_out(); }