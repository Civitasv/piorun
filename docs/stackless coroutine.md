# 无栈协程

![coroutine](images/coroutine.png)

协程有两种形式：Stackfill and Stackless

C++ 20 实现的是无栈协程，其中的数据(coroutine frame)存储在堆上。

## Keywords

A coroutine is any function that contains a `co_return`, `co_yield` or `co_await`.

|  关键字   |  作用  | 状态      |
| :-------: | :----: | :-------- |
| co_yield  | output | suspended |
| co_return | output | ended     |
| co_await  | input  | suspended |

- **co_yield 和 co_await 使协程处于挂起状态**
- **当我们想在协程执行中间为协程指定不同的入参，我们可以使用 _co_await_。**
- **co_return 能够结束协程的执行**

## 组成元素

In C++, a coroutine consists of:

- A wrapper type. This is the return type of the coroutine function's prototype.
- The compiler looks for a type with the exact name `promise_type` inside the return type of the coroutine (the wrapper type).
- An awaitable type that comes into play once we use `co_await`.

A coroutine in C++ is an finite state machine (FSM) that can be controlled and customized by the `promise_type`.

## Task vs Generator

- Task is A coroutine that does a job without returning a value (withour co_yield or co_return, maybe only co_await).
- Generator is A coroutine that does a job and returns a value (either by co_return or co_yield).

## Awaiter

`co_await` 后跟一个等待器，它需要定义以下成员函数：

- `await_ready`: if `await_ready` returns false, it means that an await expression always suspends as it waits for its value, else it means that an await expression never suspends.
- `await_suspend`: 指定协程挂起时的行为，接收 `std::coroutine_handle<>` 为参数
- `await_resume`: 在协程被恢复（resume）时调用的函数

```cpp
struct awaitable {
    bool await_ready();
    // one of:
    void await_suspend(std::coroutine_handle<>) {}
    bool await_suspend(std::coroutine_handle<>) {}
    std::coroutine_handle<>
        await_suspend(std::coroutine_handle<>) {}
    void await_resume() {}
};
```

## Functions of promise_type

- `initial_suspend`: 指定协程在启动时是否应挂起
- `final_suspend`: 指定协程在完成时是否应挂起
- `get_return_object`: 指定函数返回的数据类型
- `unhandled_exception`: 指定协程发生异常时的行为
- `yield_value`: 对应于 `co_yield xxx`
- `return_value`: 对应于 `co_return xxx`
- `return_void`: 对应于 `co_return`
- `await_transform`: 对应于 `co_await xxx`，不过我认为一般用上面的 awaiter 就可以了

## co_await task 流程

1. 执行 await_ready
2. 执行 await_suspend，存储 handler
3. 转移所有权
4. ...
5. 执行 promise_type.final_suspend
6. 执行 await_resume

## Reference

- <https://itnext.io/c-20-coroutines-complete-guide-7c3fc08db89d>
