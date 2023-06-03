#include "core/thread.h"

namespace pio {

static thread_local Thread* this_thread = nullptr;

static thread_local std::string this_thread_name = "UNDEFINED";

// TODO(horse-dog): 之后整合到其他文件
/**
 * @brief 获取当前线程的系统线程标识
 * 
 * @return pid_t 
 */
pid_t GetThreadSystemId()
{ return syscall(SYS_gettid); }

// TODO(horse-dog): 线程相关的系统日志
// static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Thread* Thread::GetCurrentThread() 
{ return this_thread; }

pid_t Thread::GetCurrentThreadId()
{ return GetThreadSystemId(); }

const std::string& Thread::GetCurrentThreadName() 
{ return this_thread_name; }

void Thread::SetCurrentThreadName(const std::string& name) {
  if (name.empty()) return;

  if (this_thread != nullptr) {
    this_thread->thread_name_ = name;
  }
  this_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name)
    : cb_(cb), thread_name_(name), system_id_(-1), thread_(0) {
  if (name.empty()) thread_name_ = "UNDEFINED";
  
  int rt = pthread_create(&thread_, nullptr, &Thread::run, this);
  if (rt != 0) {
    // TODO(horse-dog): 创建线程失败，日志输出异常
    // SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
    //         << " name=" << name;
    throw std::logic_error("pthread_create error");
  }

  semaphore_.Wait();
}

Thread::~Thread() {
  if (thread_ != 0) pthread_detach(thread_);
}

void Thread::Join() {
  if (thread_ != 0) {
    int rt = pthread_join(thread_, nullptr);
    if (rt != 0) {
      // TODO(horse-dog): Join失败，日志
      // SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
      //           << " name=" << m_name;
      throw std::logic_error("pthread_join error");
    }
    thread_ = 0;
  }
}

void* Thread::run(void* arg) {
  Thread* thread = static_cast<Thread*>(arg);
  this_thread = thread;
  this_thread_name = thread->thread_name_;
  thread->system_id_ = GetThreadSystemId();
  pthread_setname_np(pthread_self(), thread->thread_name_.substr(0, 15).c_str());
  
  std::function<void()> cb;
  cb.swap(thread->cb_);

  thread->semaphore_.Signal();
  cb();

  return 0;
}

}  // namespace pio