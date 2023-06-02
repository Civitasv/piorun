# 日志系统

目前日志系统支持:

1. 输出到控制台或输出到文件
2. 格式化输出，类似于 `printf`

## API

```cpp
  /**
   * @brief 创建日志实例，输出到文件
   *
   * @param logpath 日志输出文件
   * @return Ref<Logger> 该日志实例可以在多个线程中共享，
   *         引用数量为0时会被自动回收
   */
  static Ref<Logger> Create(const std::string& logpath);

  /**
   * @brief 创建日志实例，输出到标准输出
   *
   * @return Ref<Logger> 该日志实例可以在多个线程中共享，
   *         引用数量为0时会被自动回收
   */
  static Ref<Logger> Create();

  template <typename... T>
  void Info(const char* __restrict__ fmt, const T&... msg)

  template <typename... T>
  void Warning(const char* __restrict__ fmt, const T&... msg);

  template <typename... T>
  void Error(const char* __restrict__ fmt, const T&... msg);

  template <typename... T>
  void Fatal(const char* __restrict__ fmt, const T&... msg);
```

## 输出到控制台

```cpp
int main() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  logger->Info("This is a test for %s", "info");
  logger->Warning("This is a test for %s", "warning");
  logger->Error("This is a test %s", "error");
  logger->Fatal("This is a test %s", "fatal");
}
```

```txt
[Info] This is a test for info
[Warning] This is a test for warning
[Error] This is a test error
[Fatal] This is a test fatal
```

## 输出到文件

```cpp
int main() {
  using namespace pio::logger;
  auto logger = Logger::Create("piorun_test.log"); // 事实上，只有这里被修改了

  logger->Info("This is a test for %s", "info");
  logger->Warning("This is a test for %s", "warning");
  logger->Error("This is a test %s", "error");
  logger->Fatal("This is a test %s", "fatal");
}
```

piorun_test.log:

```txt
[Info] This is a test for info
[Warning] This is a test for warning
[Error] This is a test error
[Fatal] This is a test fatal
```
