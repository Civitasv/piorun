# 日志系统

目前日志系统支持:

1. 输出到控制台或输出到文件
2. 格式化输出，类似于 `printf`
3. 流式输出，类似于 `cout`

## 构造

### API

```cpp
  /**
   * @brief 创建日志实例，输出到文件
   *
   * @param logpath 日志输出文件
   * @param min_log_level 日志级别下限
   * @return Ref<Logger> 该日志实例可以在多个线程中共享，
   *         引用数量为0时会被自动回收
   */
  static Ref<Logger> Create(const std::string& logpath, Level min_log_level = Level::Verbose);

  /**
   * @brief 创建日志实例，输出到标准输出
   *
   * @param min_log_level 日志级别下限
   * @return Ref<Logger> 该日志实例可以在多个线程中共享，
   *         引用数量为0时会被自动回收
   */
  static Ref<Logger> Create(Level min_log_level = Level::Verbose);
```

提供两种构造策略，如果构造时提供了文件路径，则将日志输出到文件，否则将日志输出到控制台。

### 输出到控制台

```cpp
int main() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  logger->VerboseF("This is a test for %s", "verbose");
  logger->DebugF("This is a test for %s", "debug");
  logger->InfoF("This is a test for %s", "info");
  logger->WarningF("This is a test for %s", "warning");
  logger->ErrorF("This is a test %s", "error");
  logger->FatalF("This is a test %s", "fatal");
}
```

```txt
[Verbose] This is a test for verbose
[Debug] This is a test for debug
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

  logger->VerboseF("This is a test for %s", "verbose");
  logger->DebugF("This is a test for %s", "debug");
  logger->InfoF("This is a test for %s", "info");
  logger->WarningF("This is a test for %s", "warning");
  logger->ErrorF("This is a test %s", "error");
  logger->FatalF("This is a test %s", "fatal");
}
```

piorun_test.log:

```txt
[Verbose] This is a test for verbose
[Debug] This is a test for debug
[Info] This is a test for info
[Warning] This is a test for warning
[Error] This is a test error
[Fatal] This is a test fatal
```

## 类似于 C 语言中 printf 的格式化输出

该方式目的是提供对 C 语言数据类型以及相关 C 语言依赖库的输出支持。

### API

```cpp
  template <typename... T>
  void VerboseF(const char* __restrict__ fmt, const T&... msg)

  template <typename... T>
  void DebugF(const char* __restrict__ fmt, const T&... msg)

  template <typename... T>
  void InfoF(const char* __restrict__ fmt, const T&... msg)

  template <typename... T>
  void WarningF(const char* __restrict__ fmt, const T&... msg);

  template <typename... T>
  void ErrorF(const char* __restrict__ fmt, const T&... msg);

  template <typename... T>
  void FatalF(const char* __restrict__ fmt, const T&... msg);
```

### Example

```cpp
void basic_test_with_format() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  logger->VerboseF("This is a test for %s", "verbose");
  logger->DebugF("This is a test for %s", "debug");
  logger->InfoF("This is a test for %s", "info");
  logger->WarningF("This is a test for %s", "warning");
  logger->ErrorF("This is a test for %s", "error");
  logger->FatalF("This is a test for %s", "fatal");
}
```

```txt
[Verbose] This is a test for verbose
[Debug] This is a test for debug
[Info] This is a test for info
[Warning] This is a test for warning
[Error] This is a test for error
[Fatal] This is a test for fatal
```

## 更适合 C++ 的非格式化输出

为什么说更适合 C++ 呢，假设某个类 `A` 重载了 `<<` 运算符，那么我们就可以直接使用 `logger->Info(a)` 进行打印了，无需首先将其 `ToString`，再传递给 `printf` 的 `%s`，简单来说，就是更 C++ 了。

### API

```cpp
  template <typename... T>
  void Verbose(const T&... msg);

  template <typename... T>
  void Debug(const T&... msg);

  template <typename... T>
  void Info(const T&... msg);

  template <typename... T>
  void Warning(const T&... msg);

  template <typename... T>
  void Error(const T&... msg);

  template <typename... T>
  void Fatal(const T&... msg);
```

### Example

```cpp
void basic_test_without_format() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  logger->Verbose("This is a test for ", "verbose");
  logger->Debug("This is a test for ", "debug");
  logger->Info("This is a test for ", "info");
  logger->Warning("This is a test for ", "warning");
  logger->Error("This is a test for ", "error");
  logger->Fatal("This is a test for ", "fatal");
}
```

```txt
[Verbose] This is a test for verbose
[Debug] This is a test for debug
[Info] This is a test for info
[Warning] This is a test for warning
[Error] This is a test for error
[Fatal] This is a test for fatal
```

## 输出级别控制

目前存在两种方式对输出级别进行控制：

1. 默认方式：设置使用日志级别下限，所有级别大于等于该下限的日志会输出
2. 设置日志掩码，默认情况下，所有级别的日志均会输出

对于方式一：

### API

```cpp
  void set_min_log_level(Level min_log_level);
```

### Example

```cpp
static void test_min_log_level() {
  using namespace pio;
  auto logger = Logger::Create();
  // or: auto logger = Logger::Create(Logger::WARNING);

  logger->set_min_log_level(Logger::WARNING);

  logger->Info("This won't show");
  logger->Warning("This will show");
  logger->Error("This will show");
  logger->Fatal("This will show");
}
```

对于方式二：

### API

```cpp
  void set_use_mask(bool use_mask);
  void set_mask(uint32_t mask);
```

### Example

```cpp
static void test_log_mask() {
  using namespace pio;
  auto logger = Logger::Create();

  logger->set_use_mask(true); // 必须设置 use_mask_ 为 true
  logger->set_mask(Logger::INFO | Logger::ERROR | Logger::FATAL);

  logger->Info("This will show");
  logger->Warning("This won't show");
  logger->Error("This will show");
  logger->Fatal("This will show");
}
```

## Bonus: 提供流式输出方式

流式输出方式指类似于 `logger << A << B << "TEST" << '\n` 的方式，不同于 `logger->Info` 的是，流式输出方式没有日志的级别设置，只是简单的将用户输入进去的输出到文件或者控制台中。

### Example

```cpp
void test_stream_out() {
  using namespace pio::logger;
  auto logger = Logger::Create();

  logger << "TEST "
         << "For "
         << "Stream "
         << "Out " << 1 << '\n';
}
```

```txt
TEST For Stream Out 1
```

我认为比较适合于一些比较简单的场景。
