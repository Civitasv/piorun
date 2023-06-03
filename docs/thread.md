## 线程模块实现

#### Semaphore

- 不采用c++20 counting_semaphore，原因是counting_semaphore是一个模板，模板参数作为信号量容量

```cpp
// 信号量容量为10，初始值为2
std::counting_semaphore<10> sem(2);
```

- 互斥量，读写互斥量，lock_guard，unique_lock，shared_lock 使用c++标准库

- 额外封装了用户态自旋锁和CAS锁，自旋锁是对pthread_spinlock_t的简单封装，CAS则使用std::atomic_flag实现

- nullmutex, nullrwmutx, FiberSemaphore: 空锁用于调试？我们就不实现了，FiberSemaphore是协程级信号量，暂不实现(感觉放在协程模块较好)

#### Thread

- 对pthread的简单封装

- 增加了线程名称功能

- 区别线程系统标识（一个pid_t类型）和线程ID（pthread_t类型）

- top -H -p [进程ID] 可以查看某进程下的线程

#### 问题

- bin目录太长，个人觉得bin/debug, bin/release就够了

- 模块文件组织问题

- 开发文档：每个模块实现的同时都简单写一个文档，方便其他人看

- api：尽量使用标准库api，除非有特殊需求或者特定场景的性能瓶颈，例如 RWLock、Ref 等直接用std::shared_mutex等

- 第三方库：建议不要使用，yaml库如果可以直接在cmake中配置那就用一下吧，以后有想用的库的时候一起讨论一下看有没有用的必要

- api文档：用Doxygen自动生成？
