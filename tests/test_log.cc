#include <iostream>

#include "core/log.h"

int main() {
  using namespace pio::logger;
  auto logger = Logger::Create("piorun_test.log");

  logger->Info("This is a test");
  logger->Warning("This is a test");
  logger->Error("This is a test");
  logger->Fatal("This is a test");
}