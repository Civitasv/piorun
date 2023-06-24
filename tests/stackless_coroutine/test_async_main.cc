#include <iostream>

#include "coroutine/task/main.h"

using namespace pio;

task::Main AsyncMain(int argc, char* argv[]) {
  // 这里面我们就可以使用 co_await 其他协程了
  std::cout << "Arguments: \n";
  for (int i = 0; i < argc; i++) {
    std::cout << "\t" << argv[i] << "\n";
  }
  co_return 0;  // 控制权交给调用方
}

int main(int argc, char* argv[]) { return AsyncMain(argc, argv); }