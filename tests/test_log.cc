#include <iostream>

#include "core/log.h"

int main() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  logger->Info("This is a test for %s", "info");
  logger->Warning("This is a test for %s", "warning");
  logger->Error("This is a test %s", "error");
  logger->Fatal("This is a test %s", "fatal");
}