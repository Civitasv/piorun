#### 有栈协程

- 每个协程栈大小为128KB，如果需要在协程中分配数组，强烈建议在堆中创建以避免协程栈溢出

- 协程切换：Resume、Yield。没有对浮点数环境和信号环境上下文进行保存和切换。

#### 用户接口

```cpp

// 提交一个异步任务 callable.
go callable;

namespace pio::this_fiber {

// 获取当前协程的ID
uint64_t get_id();

// 获取当前线程的ID（该ID由协程框架分配，而非内核ID）
int get_thread_id();

// 获取当前协程句柄
pio::fiber::Fiber* co_self();

// yield当前协程（之后调度器会在某一时刻自动唤醒它，类似于std::this_thread::yield）
void yield();

// 使能当前协程的阻塞系统调用的hook
void enable_system_hook();

// 关闭当前协程的阻塞系统调用的hook
void disable_system_hook();

// 睡眠当前协程
void sleep_for(const std::chrono::milliseconds& ms);

// 睡眠当前协程
void sleep_until(const std::chrono::high_resolution_clock::time_point& time_point);


}
```

#### 调度器设计

- 多线程，多协程模式，main函数执行所在的主线程最后也将成为一个工作线程。

- 协程一旦在某个线程上开始运行，它的Yield、Resume操作不会跨线程。

- 采用自旋锁保证线程安全。

- 当前进程类没有任何协程可用时，进程将自动退出。

- 每个线程中的新产生的任务将暂存到线程本地的待提交任务队列：raisedTasks，然后再loop循环中一次性把这些任务全部提交到全局任务队列 commTasks，以减小锁的粒度。

- 进程持有一个令牌 turn，该令牌标志着下一次将由哪一个线程去全局任务队列中获取任务，当某个线程持有该令牌时，该线程将把当前 commTasks 中的任务全部取出，然后随机更新令牌的值，然后创建协程去执行这些任务。

- 从 commTasks中取出任务时，直接使用 std::deque::swap 函数，减小锁的粒度

#### System Hook

- 对常用的阻塞系统调用进行hook，确保用户可以以同步的方式正常使用这些api，而不需要去关注协程内部的yield和resume的细节。

#### 并发支持

```cpp
namespace pio::fiber {

// 协程间互斥锁
class mutex;

// 协程间读写锁(写优先)
class shared_mutex;

// 协程间条件变量
class condition_variable;

// 协程间信号量
class semaphore;

// 协程间管道，支持 >>、<< 运算符重载
class channel;

}
```

#### 超时

举例：设置读事件超时

```cpp
timeval timeout;
timeout.tv_sec = 60;
timeout.tv_usec = 0;
setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
```

#### 定时事件

举例：10秒后打印 hello world！

```cpp
go [] {
  pio::this_fiber::sleep_for(10s);
  printf("hello world!\n");
}
```

#### 协程池

- 每个任务函数是运行在协程之上的，而每个协程需要从堆中分配128K的栈空间，因此将协程池化，当池中协程数量不足时，新建一批协程以供使用，当协程中的任务运行完毕后，将协程归还给协程池。

#### 测试

- test_fiber: 测试协程基本切换功能。

- test_fiber_cond: 测试条件变量

- test_fiber_rw: 测试读写锁

- test_fiber_sem: 测试信号量

- test_fiber_echoserver: 测试回声服务器

- test_imsystem: 即时通讯系统

#### 关于嵌套调用

```cpp
go [] {
  LogInfo("first");
  // 这里不是新建协程然后resume这个协程，而是将这个任务提交到任务队列
  // 因此下面这个go不会立刻执行（有一定延时）
  // 这也表面协程的调用链长度最多只有2：主协程 + 被调度的协程
  // 但调度栈仍然保留了128个槽位，方便以后修改
  go [] {
    LogInfo("second");
    go [] {
      this_fiber::sleep_for(15s);
      LogInfo("third");
    };
  };
};
```

#### 关于共享栈

- 共享栈有利于减少协程数量很多的时候，协程栈对内存的使用

- 但共享栈模式会使得上下文切换变慢（可能会多出栈内存的拷贝过程）

- 共享栈模式需要在进程一开始就指定共享栈数组的大小，不方便动态扩充

- 综上，本框架实现未采用该方案

#### 关于accept

- 如果需要使得accept在一个线程单独运行，可以通过修改accept函数的hook行为实现，目前暂时将accept调用所在的协程于其他协程同等对待。
