#include <chrono>
#include <iostream>
#include <thread>

#include "core/log.h"

static void test_with_format() {
  using namespace pio;
  auto logger = Logger::Create();

  logger->InfoF("This is a test for %s", "info");
  logger->WarningF("This is a test for %s", "warning");
  logger->ErrorF("This is a test for %s", "error");
  logger->FatalF("This is a test for %s", "fatal");
}

static void test_without_format() {
  using namespace pio;
  auto logger = Logger::Create();

  logger->Info("This is a test for ", "info");
  logger->Warning("This is a test for ", "warning");
  logger->Error("This is a test for ", "error");
  logger->Fatal("This is a test for ", "fatal");
}

static void test_in_thread() {
  using namespace pio;
  auto logger = Logger::Create();

  auto func = [logger](const std::string& name) {
    for (int i = 0; i < 100; i++) {
      if (name == "A")
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
      else if (name == "B")
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

      logger->Info("This is a test for thread ", name, i);
    }
  };

  // 应该每三秒输出一次 A，每五秒输出一次 B
  std::thread t1(func, "A");
  std::thread t2(func, "B");

  t1.join();
  t2.join();
}

static void test_stream_out() {
  using namespace pio;
  auto logger = Logger::Create();

  logger << "TEST "
         << "For "
         << "Stream "
         << "Out " << 1 << '\n';
}

static void test_log_mask() {
  using namespace pio;
  auto logger = Logger::Create();

  logger->set_mask(Logger::ERROR | Logger::FATAL | Logger::INFO);

  logger->Info("This will show");
  logger->Warning("This won't show");
  logger->Error("This will show");
  logger->Fatal("This will show");
}

int main() { test_log_mask(); }