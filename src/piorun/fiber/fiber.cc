#include <assert.h>
#include <errno.h>
#include <fiber/fiber.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/time.h>

#include <thread>

thread_local bool isaccept = false;

namespace pio::fiber {

extern "C" {

extern void coctx_swap(FiberContext*, FiberContext*) asm("coctx_swap");
}

/**
 * @brief 获取当前系统时间（毫秒级）
 *
 * @return unsigned long long
 */
static unsigned long long GetTickMS() {
  // 1秒 = 1000毫秒 = 1000'000微秒 = 1000'000'000纳秒
  // timeval结构体由秒 + 微秒组成
  struct timeval now = {0};
  gettimeofday(&now, NULL);
  unsigned long long u = now.tv_sec;
  u *= 1000;
  u += now.tv_usec / 1000;
  return u;
}

// 超时事件结点组成的链表
struct StTimeoutItemLink;

// 超时事件结点
struct StTimeoutItem {
  using OnPreparePfn_t = void (*)(StTimeoutItem*, epoll_event& ev,
                                  StTimeoutItemLink*);
  using OnProcessPfn_t = void (*)(StTimeoutItem*);

  StTimeoutItem* pPrev;     /*> 指向上一个结点 */
  StTimeoutItem* pNext;     /*> 指向下一个结点 */
  StTimeoutItemLink* pLink; /*> 指向本结点所属的链表 */

  uint64_t ullExpireTime; /*> 超时时间 */
  uint64_t isDelayActive; /*> 是否是在之后的轮子才超时 */

  OnPreparePfn_t pfnPrepare; /*> 预处理函数，在eventloop中会被调用 */
  OnProcessPfn_t pfnProcess; /*> 处理函数，在eventloop中会被调用 */

  void* pArg; /*> 携带一些信息，例如可以用于指向该超时结点所在的协程 */
  bool bTimeout; /*> 本结点是否已经超时 */

  StTimeoutItem()
      : pPrev(),
        pNext(),
        pLink(),
        ullExpireTime(),
        isDelayActive(),
        pfnPrepare(),
        pfnProcess(),
        pArg(),
        bTimeout(false) {}

  ~StTimeoutItem() {}

  /**
   * @brief 将本结点从所属的链表中移除.
   */
  void RemoveFromLink();
};

struct StTimeoutItemLink {
  StTimeoutItem* head_; /*> 指向链表头部 */
  StTimeoutItem* tail_; /*> 指向链表尾部 */

  StTimeoutItemLink() : head_(), tail_() {}

  ~StTimeoutItemLink() {}

  /**
   * @brief 在链表尾部追加超时事件结点
   *
   * @param ap 待追加的超时事件结点
   */
  void AddTail(StTimeoutItem* ap) {
    // 如果结点ap已经有了主人（链表），直接返回
    if (ap->pLink) return;
    // 如果链表apLink有尾结点（即链表非空）
    if (this->tail_) {
      this->tail_->pNext = ap;
      ap->pNext = NULL;
      ap->pPrev = this->tail_;
      // 这里记得更新链表的尾结点
      this->tail_ = ap;
    } else {  // 链表还是空的(ˉ▽ˉ；)...
      this->head_ = this->tail_ = ap;
      ap->pNext = ap->pPrev = NULL;  // 这里说明实现的是无环的双链表
    }
    // 最后记得设置parent
    ap->pLink = this;
  }

  /**
   * @brief 弹出链表头部的超时事件结点
   */
  void inline PopHead() {
    if (this->head_ == nullptr) return;
    auto lp = this->head_;
    if (this->head_ == this->tail_) {
      this->head_ = this->tail_ = NULL;
    } else {
      this->head_ = this->head_->pNext;
    }

    lp->pPrev = lp->pNext = NULL;
    lp->pLink = NULL;

    if (this->head_ != nullptr) {
      this->head_->pPrev = NULL;
    }
  }

  /**
   * @brief 将链表 @p apOther splice到本链表中
   *
   * @param apOther 另一个链表
   */
  void inline Join(StTimeoutItemLink* apOther) {
    // 如果apOther链表为空，那不需要Join了
    if (!apOther->head_) {
      return;
    }

    // apOther的所有结点的parent设置为apLink
    auto lp = apOther->head_;
    while (lp) {
      lp->pLink = this;
      lp = lp->pNext;
    }

    lp = apOther->head_;

    // 把apOther链接到apLink尾部
    if (this->tail_) {
      this->tail_->pNext = lp;
      lp->pPrev = this->tail_;
      this->tail_ = apOther->tail_;
    } else {
      this->head_ = apOther->head_;
      this->tail_ = apOther->tail_;
    }

    // apOther现在被掏空了
    apOther->head_ = apOther->tail_ = nullptr;
  }
};

// 时间轮定时器容器
struct StTimeout {
  StTimeoutItemLink* pItems_; /*> 时间轮，是一个由多个超时事件链表组成的数组 */
  int iItemSize_; /*> 时间轮的大小，默认为60*1000 */

  uint64_t ullStart_;  /*> 上一次取超时事件的绝对时间 */
  int64_t llStartIdx_; /*> 上一次取超时事件的槽号范围的最大值 +
                          1，即下次取超时事件的开始槽号 */

  StTimeout(int iSize)
      : pItems_(), iItemSize_(iSize), ullStart_(GetTickMS()), llStartIdx_() {
    pItems_ = new StTimeoutItemLink[iSize];
  }

  ~StTimeout() {
    delete[] pItems_;
    pItems_ = nullptr;
    iItemSize_ = 0;
    ullStart_ = 0;
    llStartIdx_ = 0;
  }

  /**
   * @brief 新注册一个超时事件结点
   *
   * @param apItem 新的超时事件结点
   * @param allNow 当前时间(ms)
   * @return int 插入成功时返回0, 插入失败时返回-1
   */
  int AddTimeout(StTimeoutItem* apItem, uint64_t allNow) {
    if (this->ullStart_ == 0) {
      this->ullStart_ = allNow;
      this->llStartIdx_ = 0;
    }

    // 计算时间差，单位为ms
    auto diff = apItem->ullExpireTime - this->ullStart_;

    // 比上一次取超时事件的时间还要长超过了一分钟（60*1000ms)，（溢出了。。。）报错
    // 不过这里又改为了将超时时间设置为最长支持的时间，然后添加。。。
    // 增加 isDelayActive 实现长时间定时器
    apItem->isDelayActive = diff >= iItemSize_ ? 1 : 0;

    // if (diff >= (uint64_t)this->iItemSize_) {
    //   diff = this->iItemSize_ - 1;
    // }

    /**
     * 返回-1的情况：
     * 1. 时间轮定时器容器已经步过了 allNow 时刻.
     * 2. 想要添加一个超时结点，然而该结点在添加前就超时了.
     */
    if (allNow < this->ullStart_ || apItem->ullExpireTime < allNow) {
      return -1;
    }

    // 好的，把这个新结点添加到时间轮中对应的槽指向的链表的尾部
    this->pItems_[(llStartIdx_ + diff) % iItemSize_].AddTail(apItem);
    return 0;
  }

  /**
   * @brief 取出所有截至目前为止超时的事件，将它们append到 @p apResult 中.
   *
   * @param allNow 当前时间(ms)
   * @param apResult 存储超时事件的链表
   */
  void TakeAllTimeout(uint64_t allNow, StTimeoutItemLink* apResult) {
    if (this->ullStart_ == 0) {
      this->ullStart_ = allNow;
      this->llStartIdx_ = 0;
    }

    if (allNow < this->ullStart_) {
      return;
    }

    // 超时的时间槽的数量可以通过时间差除以粒度得到
    int cnt = allNow - this->ullStart_ + 1;

    // 如果超时的时间槽的数量超过了时间轮的最大容量，把它限制为这个最大容量
    if (cnt > this->iItemSize_) {
      cnt = this->iItemSize_;
    }

    // 如果cnt小于0，肯定有问题（但是这里感觉已经肯定不会小于0了。。。）
    if (cnt < 0) {
      return;
    }

    // 好的，现在开始遍历这些时间槽各自指向的链表吧，把这些链表全部splice到apResult
    for (int i = 0; i < cnt; i++) {
      int idx = (this->llStartIdx_ + i) % this->iItemSize_;
      apResult->Join(&this->pItems_[idx]);
    }

    // 现在更新上一次取超时事件的绝对时间和槽号
    this->ullStart_ = allNow;
    this->llStartIdx_ += cnt - 1;
  }
};

void StTimeoutItem::RemoveFromLink() {
  auto lst = this->pLink;
  if (!lst) return;
  assert(lst->head_ && lst->tail_);

  if (this == lst->head_) {
    lst->head_ = this->pNext;
    if (lst->head_) {
      lst->head_->pPrev = nullptr;
    }
  } else {
    if (this->pPrev) {
      this->pPrev->pNext = this->pNext;
    }
  }

  if (this == lst->tail_) {
    lst->tail_ = this->pPrev;
    if (lst->tail_) {
      lst->tail_->pNext = nullptr;
    }
  } else {
    this->pNext->pPrev = this->pPrev;
  }

  this->pPrev = this->pNext = nullptr;
  this->pLink = nullptr;
}

struct StCoPollItem;

struct StCoPoller : public StTimeoutItem {
  pollfd*
      fds; /*> pollfd数组，用于注册和获取事件，每个元素与一个Poll结点相对应 */
  nfds_t nfds;              /*> fds数组的长度 */
  StCoPollItem* pPollItems; /* 该管理者所管理的Poll结点所组成的数组 */
  int iAllEventDetach; /*>
                          该Poll结构体等待的事件数组中，只有有一个事件到达了，该标识设置为1
                        */
  int iEpollFd;        /*> epfd */
  int iRaiseCnt;       /*> poll的active事件个数 */

  StCoPoller(int epfd, nfds_t numfds)
      : fds(),
        nfds(numfds),
        pPollItems(),
        iAllEventDetach(),
        iEpollFd(epfd),
        iRaiseCnt() {
    fds = new pollfd[numfds];
  }

  ~StCoPoller() { delete[] fds; }

  static uint32_t PollEvent2Epoll(short events) {
    uint32_t e = 0;
    if (events & POLLIN) e |= EPOLLIN;
    if (events & POLLOUT) e |= EPOLLOUT;
    if (events & POLLHUP) e |= EPOLLHUP;
    if (events & POLLERR) e |= EPOLLERR;
    if (events & POLLRDNORM) e |= EPOLLRDNORM;
    if (events & POLLWRNORM) e |= EPOLLWRNORM;
    return e;
  }

  static short EpollEvent2Poll(uint32_t events) {
    short e = 0;
    if (events & EPOLLIN) e |= POLLIN;
    if (events & EPOLLOUT) e |= POLLOUT;
    if (events & EPOLLHUP) e |= POLLHUP;
    if (events & EPOLLERR) e |= POLLERR;
    if (events & EPOLLRDNORM) e |= POLLRDNORM;
    if (events & EPOLLWRNORM) e |= POLLWRNORM;
    return e;
  }
};

struct StCoPollItem : public StTimeoutItem {
  pollfd* pSelf;       /*> 指向该Poll结点对应的pollfd */
  StCoPoller* pPoll;   /*> 指向管理该Poll结点的Poll结构体 */
  epoll_event stEvent; /*> 用于存放关心的epoll事件 */

  StCoPollItem() : pSelf(), pPoll(), stEvent() {}
};

/**
 * @brief poll process function, 唤醒超时事件结点 @p ap 所在的协程.
 *
 * @param ap 超时事件结点
 */
static void OnPollProcess(StTimeoutItem* ap) {
  Fiber* co = (Fiber*)ap->pArg;
  co->Resume();
}

/**
 * @brief poll prepare function,
 *
 * @param ap 超时事件结点
 * @param e  epoll_event
 * @param active 到达事件队列
 */
static void OnPollPrepare(StTimeoutItem* ap, epoll_event& e,
                          StTimeoutItemLink* active) {
  StCoPollItem* lp = (StCoPollItem*)ap;
  lp->pSelf->revents = StCoPoller::EpollEvent2Poll(e.events);

  StCoPoller* pPoll = lp->pPoll;
  pPoll->iRaiseCnt++;

  if (!pPoll->iAllEventDetach) {
    pPoll->iAllEventDetach = 1;
    pPoll->RemoveFromLink();
    active->AddTail(pPoll);
  }
}

class FiberEnvironment {
  friend class Fiber;
  friend class FiberScheduler;

 public:
  Fiber* pCallStack_[128];         /*> 协程调用栈 */
  int callStackSize_;              /*> 当前调用栈长度 */
  int EpollFd_;                    /*> epfd */
  int threadId_;                   /*> 线程ID, 用于识别调度器 */
  StTimeout* pTimeWheel_;          /*> time wheel */
  StTimeoutItemLink pActiveList_;  /*> 到达事件缓存 */
  StTimeoutItemLink pTimeoutList_; /*> 超时事件缓存 */
  epoll_event* epollEvents_;       /*> epoll_event数组 */
  size_t eventsLength_;            /*> epoll_event数组的大小 */
  size_t currentFiberCount_; /*> 本线程目前正在运行的协程数量(不包含主协程) */
  std::deque<std::function<void()>>
      raisedTasks_; /*> 本轮loop新产生的任务组成的队列 */
  std::deque<Fiber*> userYieldFiberQ_; /*> 用户手动yield的协程的队列 */
  SpinLock
      lockForSyncSignalFiberQ_; /*>
                                   用于线程间互斥地访问被同步类唤醒的协程组成的队列
                                 */
  std::deque<Fiber*> syncSignalFiberQ_; /*> 被同步类唤醒的协程组成的队列 */
  std::deque<Fiber*> fiberPool_; /*> 协程池 */

  static const int EPOLL_SIZE_ = 1024 * 10; /*> epoll_wait最大支持的事件数 */

 private:
  FiberEnvironment()
      : pCallStack_(),
        callStackSize_(),
        pActiveList_(),
        pTimeoutList_(),
        currentFiberCount_(0) {
    Fiber* self = new Fiber(true);
    // 入栈主协程
    pCallStack_[callStackSize_++] = self;
    EpollFd_ = epoll_create(1);
    pTimeWheel_ = new StTimeout(60 * 1000); /* one wheel is 60 seconds */
    epollEvents_ = new epoll_event[EPOLL_SIZE_];
    eventsLength_ = EPOLL_SIZE_;
  }

  ~FiberEnvironment() {
    if (threadId_ == -1) {
      threadRoutine(0, &MultiThreadFiberScheduler::GetInstance());
    }
    // close(EpollFd_); could recycle by system.
    Fiber* mainCo = pCallStack_[0];
    delete mainCo;
    delete pTimeWheel_;
    delete[] epollEvents_;
    for (auto x : fiberPool_) {
      delete x;
    }
    pTimeWheel_ = nullptr;
    epollEvents_ = nullptr;
    eventsLength_ = 0;
    callStackSize_ = 0;
    currentFiberCount_ = 0;
  }

 public:
  static FiberEnvironment* GetInstance() {
    thread_local FiberEnvironment env;
    return &env;
  }

  int EpollWait(int timeout) {
    return epoll_wait(EpollFd_, epollEvents_, eventsLength_, timeout);
  }

  Fiber* GetFiberFromPool() {
    if (fiberPool_.empty()) {
      fiberPool_.resize(1024);
      for (auto&& x : fiberPool_) {
        x = new Fiber();
        x->cCreateByEnv = 1;
      }
    }

    Fiber* x = fiberPool_.front();
    fiberPool_.pop_front();
    return x;
  }

  void RecycleFiberToPool(Fiber* fiber) { fiberPool_.push_back(fiber); }
};

int GoRoutine(Fiber* co, void*) {
  co->env_->currentFiberCount_ += 1;
  if (co->pfn_ != nullptr) {
    co->pfn_();
  }
  co->cEnd_ = 1;
  if (co->cCreateByEnv) {
    co->env_->RecycleFiberToPool(co);
  }
  co->env_->currentFiberCount_ -= 1;
  co->Yield();
  return 0;
}

Fiber::Fiber(bool)
    : env_(nullptr),
      pfn_(nullptr),
      cStart_(0),
      cEnd_(0),
      cIsMain_(1),
      cEnableSysHook_(0),
      cCreateByEnv(0) {}

Fiber::Fiber(const std::function<void()>& pfn)
    : env_(FiberEnvironment::GetInstance()),
      pfn_(pfn),
      cStart_(0),
      cEnd_(0),
      cIsMain_(0),
      cEnableSysHook_(1),
      cCreateByEnv() {}

Fiber::~Fiber() {
  pfn_ = nullptr;
  cStart_ = cEnd_ = 0;
}

void Fiber::Yield() {
  Fiber* last = env_->pCallStack_[env_->callStackSize_ - 2];
  env_->callStackSize_--;
  SwapContext(last);
}

void Fiber::Resume() {
  Fiber* curr = env_->pCallStack_[env_->callStackSize_ - 1];
  if (!this->cStart_) {
    this->cStart_ = 1;
    this->ctx_.Make((void* (*)(void*, void*))GoRoutine, this, nullptr);
  }
  env_->pCallStack_[env_->callStackSize_++] = this;
  curr->SwapContext(this);
}

void Fiber::Reset(const std::function<void()>& pfn) {
  if (cIsMain_) return;
  if (cStart_ && !cEnd_) return;
  cStart_ = 0;
  cEnd_ = 0;
  this->pfn_ = pfn;
}

void Fiber::Reset(std::function<void()>&& pfn) {
  if (cIsMain_) return;
  if (cStart_ && !cEnd_) return;
  cStart_ = 0;
  cEnd_ = 0;
  this->pfn_ = std::move(pfn);
}

void Fiber::SwapContext(Fiber* pendingCo) {
  coctx_swap(&(this->ctx_), &(pendingCo->ctx_));
}

void condition_variable::wait(std::unique_lock<fiber::mutex>& lock) {
  this->mtx.Lock();
  waiters.push_back(this_fiber::co_self());
  this->mtx.Unlock();
  lock.unlock();
  this_fiber::co_self()->Yield();
  lock.lock();
}

void condition_variable::notify_one() {
  Fiber* fb = nullptr;
  mtx.Lock();
  if (!waiters.empty()) {
    fb = waiters.front();
    waiters.pop_front();
    // printf("signal..., waiters size = %lu\n", waiters.size());
  }
  mtx.Unlock();
  if (fb != nullptr) {
    fb->env_->lockForSyncSignalFiberQ_.Lock();
    fb->env_->syncSignalFiberQ_.push_back(fb);
    fb->env_->lockForSyncSignalFiberQ_.Unlock();
  }
}

void condition_variable::notify_all() {
  std::deque<Fiber*> dq;
  mtx.Lock();
  if (!waiters.empty()) {
    dq.swap(waiters);
    // printf("signal..., waiters size = %lu\n", waiters.size());
  }
  mtx.Unlock();
  for (auto fiber : dq) {
    fiber->env_->lockForSyncSignalFiberQ_.Lock();
    fiber->env_->syncSignalFiberQ_.push_back(fiber);
    fiber->env_->lockForSyncSignalFiberQ_.Unlock();
  }
}

void mutex::lock() {
  mtx.Lock();
  if (locked == false) {
    locked = true;
    mtx.Unlock();
    return;
  }
  waiters.push_back(this_fiber::co_self());
  mtx.Unlock();
  this_fiber::co_self()->Yield();
}

bool mutex::try_lock() {
  mtx.Lock();
  if (locked == false) {
    locked = true;
    mtx.Unlock();
    return true;
  }
  mtx.Unlock();
  return false;
}

void mutex::unlock() {
  Fiber* fb = nullptr;
  mtx.Lock();
  if (!waiters.empty()) {
    fb = waiters.front();
    waiters.pop_front();
  } else {
    locked = false;
  }
  mtx.Unlock();
  if (fb != nullptr) {
    fb->env_->lockForSyncSignalFiberQ_.Lock();
    fb->env_->syncSignalFiberQ_.push_back(fb);
    fb->env_->lockForSyncSignalFiberQ_.Unlock();
  }
}

void shared_mutex::lock() {
  mtx.Lock();
  if (state == 0) {
    // printf("state is 0, lock succeed\n");
    state = -1;
    mtx.Unlock();
    return;
  }
  // printf("lock failed..., add to wqueue\n");
  wwaiters.push_back(this_fiber::co_self());
  mtx.Unlock();
  this_fiber::co_self()->Yield();
}

bool shared_mutex::try_lock() {
  mtx.Lock();
  if (state == 0) {
    state = -1;
    mtx.Unlock();
    return true;
  }
  mtx.Unlock();
  return false;
}

void shared_mutex::unlock() {
  Fiber* fb = nullptr;
  std::deque<Fiber*> pendingReadersQ;
  mtx.Lock();
  if (!wwaiters.empty()) {
    // printf("unlock and call another writer\n");
    fb = wwaiters.front();
    wwaiters.pop_front();
  } else if (!rwaiters.empty()) {
    // printf("unlock and call all reader\n");
    state = rwaiters.size();
    pendingReadersQ.swap(rwaiters);
  } else {
    // printf("unlock, set state to 0\n");
    state = 0;
  }
  mtx.Unlock();

  if (fb != nullptr) {
    fb->env_->lockForSyncSignalFiberQ_.Lock();
    fb->env_->syncSignalFiberQ_.push_back(fb);
    fb->env_->lockForSyncSignalFiberQ_.Unlock();
  } else if (!pendingReadersQ.empty()) {
    for (auto fiber : pendingReadersQ) {
      fiber->env_->lockForSyncSignalFiberQ_.Lock();
      fiber->env_->syncSignalFiberQ_.push_back(fiber);
      fiber->env_->lockForSyncSignalFiberQ_.Unlock();
    }
  }
}

void shared_mutex::lock_shared() {
  mtx.Lock();
  if (state >= 0 && wwaiters.empty()) {
    // printf("state = %d, and no wwaiters, lock succeed\n", state);
    state++;
    mtx.Unlock();
    return;
  }
  // if (state < 0)
  // printf("lock_shared failed..., state is -1, add to rqueue\n");
  // else
  // printf("lock_shared failed..., has wwaiters, add to rqueue\n");

  rwaiters.push_back(this_fiber::co_self());
  mtx.Unlock();
  this_fiber::co_self()->Yield();
}

bool shared_mutex::try_lock_shared() {
  mtx.Lock();
  if (state >= 0 && wwaiters.empty()) {
    state++;
    mtx.Unlock();
    return true;
  }
  mtx.Unlock();
  return false;
}

void shared_mutex::unlock_shared() {
  Fiber* fb = nullptr;
  mtx.Lock();
  if (state == 1 && !wwaiters.empty()) {
    // printf("last reader, unlock_shared and call a writer\n");
    fb = wwaiters.front();
    wwaiters.pop_front();
    state = -1;
  } else {
    state--;
    // printf("unlock_shared, state--, reader left: %d\n", state);
  }
  mtx.Unlock();
  if (fb != nullptr) {
    fb->env_->lockForSyncSignalFiberQ_.Lock();
    fb->env_->syncSignalFiberQ_.push_back(fb);
    fb->env_->lockForSyncSignalFiberQ_.Unlock();
  }
}

void semaphore::wait() {
  mtx.Lock();
  if (count > 0) {
    --count;
    mtx.Unlock();
    return;
  }
  waiters.push_back(this_fiber::co_self());
  mtx.Unlock();
  this_fiber::co_self()->Yield();
}

bool semaphore::try_wait() {
  mtx.Lock();
  if (count > 0) {
    --count;
    mtx.Unlock();
    return true;
  }
  mtx.Unlock();
  return false;
}

void semaphore::signal() {
  Fiber* fb = nullptr;
  mtx.Lock();
  if (!waiters.empty()) {
    fb = waiters.front();
    waiters.pop_front();
  } else {
    ++count;
  }
  mtx.Unlock();
  if (fb != nullptr) {
    fb->env_->lockForSyncSignalFiberQ_.Lock();
    fb->env_->syncSignalFiberQ_.push_back(fb);
    fb->env_->lockForSyncSignalFiberQ_.Unlock();
  }
}

// void FiberScheduler::Start(std::function<int(void)> pfn) {
//   auto env = FiberEnvironment::GetInstance();
//   auto tmWheel = env->pTimeWheel_;
//   auto events  = env->epollEvents_;

//   for (;;) {
//     int eventNum = env->EpollWait(1); // wait for 1 ms
//     auto  active = env->pActiveList_;
//     auto timeout = env->pTimeoutList_;

//     timeout.head_ = timeout.tail_ = nullptr;

//     // get all the arrivedNode.
//     for (int i = 0; i < eventNum; i++) {
//       auto arrivedNode = (StTimeoutItem*)events[i].data.ptr;
//       if (arrivedNode->pfnPrepare) {
//         arrivedNode->pfnPrepare(arrivedNode, events[i], &active);
//       } else {
//         active.AddTail(arrivedNode);
//       }
//     }

//     // get all the timeoutNode.
//     auto now = GetTickMS();
//     tmWheel->TakeAllTimeout(now, &timeout);

//     auto lp = timeout.head_;

//     while (lp != nullptr) {
//       lp->bTimeout = true;
//       lp = lp->pNext;
//     }

//     active.Join(&timeout);

//     lp = active.head_;

//     while (lp != nullptr) {
//       active.PopHead();
//       if (lp->bTimeout && now < lp->ullExpireTime) {
//         int ret = tmWheel->AddTimeout(lp, now);
//         if (ret == 0) {
//           lp->bTimeout = false;
//           lp = active.head_;
//           continue;
//         }
//       }
//       // deal callback.
//       if (lp->pfnProcess) lp->pfnProcess(lp);
//       lp = active.head_;
//     }

//     if (pfn != nullptr) {
//       if (pfn() == -1) {
//         break;
//       }
//     }
//   }
// }

void threadRoutine(int i, MultiThreadFiberScheduler* sc) {
  auto env = FiberEnvironment::GetInstance();
  auto tmWheel = env->pTimeWheel_;
  auto events = env->epollEvents_;
  auto&& pendingTasks = env->raisedTasks_;
  std::deque<std::function<void()>> stealedTasks;
  std::deque<Fiber*> signaledFibers;
  env->threadId_ = i;
  const auto thread_num = sc->threadNum;
  bool wannaQuit = false;

  while (true) {
    int eventNum = env->EpollWait(1);  // wait for 1 ms

    sc->mutex.lock();
    if (!sc->commTasks.empty()) {
      stealedTasks.swap(sc->commTasks);
      // sc->turn = rand() % thread_num;
      // if (wannaQuit == true) {
      //   wannaQuit = false;
      //   --sc->wannaQuitThreadCount;
      // }
    }

    // if is false, check could to true or not.
    // if (wannaQuit == false && stealedTasks.empty() &&
    //     env->currentFiberCount_ == 0) {
    //   wannaQuit = true;
    //   ++sc->wannaQuitThreadCount;
    // }

    // // check break.
    // if (sc->wannaQuitThreadCount == thread_num && stealedTasks.empty() &&
    //     env->currentFiberCount_ == 0) {
    //   sc->mutex.unlock();
    //   // printf("%d exit\n", i);
    //   break;
    // }
    sc->mutex.unlock();

    auto active = env->pActiveList_;
    auto timeout = env->pTimeoutList_;

    timeout.head_ = timeout.tail_ = nullptr;

    // get all the arrivedNode.
    for (int i = 0; i < eventNum; i++) {
      auto arrivedNode = (StTimeoutItem*)events[i].data.ptr;
      if (arrivedNode->pfnPrepare) {
        arrivedNode->pfnPrepare(arrivedNode, events[i], &active);
      } else {
        active.AddTail(arrivedNode);
      }
    }

    // get all the timeoutNode.
    auto now = GetTickMS();
    tmWheel->TakeAllTimeout(now, &timeout);

    auto lp = timeout.head_;

    while (lp != nullptr) {
      lp->bTimeout = true;
      lp = lp->pNext;
    }

    active.Join(&timeout);

    lp = active.head_;

    while (lp != nullptr) {
      active.PopHead();
      if (lp->bTimeout && now < lp->ullExpireTime) {
        int ret = tmWheel->AddTimeout(lp, now);
        if (ret == 0) {
          lp->bTimeout = false;
          lp = active.head_;
          continue;
        }
      }
      // deal callback.
      if (lp->pfnProcess) lp->pfnProcess(lp);
      lp = active.head_;
    }

    for (auto&& task : stealedTasks) {
      Fiber* fiber = env->GetFiberFromPool();
      fiber->Reset(std::move(task));
      fiber->Resume();
    }

    stealedTasks.clear();

    for (auto usrYieldFiber : env->userYieldFiberQ_) {
      usrYieldFiber->Resume();
    }

    env->userYieldFiberQ_.clear();

    env->lockForSyncSignalFiberQ_.Lock();
    if (!env->syncSignalFiberQ_.empty()) {
      // printf("//////////。。。。。\n");
    }
    signaledFibers.swap(env->syncSignalFiberQ_);
    env->lockForSyncSignalFiberQ_.Unlock();

    for (auto fb : signaledFibers) {
      fb->Resume();
    }

    signaledFibers.clear();

    // sc->mutex.Lock();
    // if (sc->commTasks.empty()) {
    //   sc->commTasks.swap(pendingTasks);
    // } else {
    //   for (auto&& task : pendingTasks) {
    //     sc->commTasks.emplace_back(std::move(task));
    //   }
    // }
    // sc->mutex.Unlock();
    // pendingTasks.clear();
  }
}

MultiThreadFiberScheduler::MultiThreadFiberScheduler(int threadNum)
    : threadNum(threadNum) {
  FiberEnvironment::GetInstance()->threadId_ = -1;
  for (int i = 1; i < threadNum; i++) {
    this->threads.emplace_back(std::bind(threadRoutine, i, this));
  }
}

MultiThreadFiberScheduler::~MultiThreadFiberScheduler() {
  for (auto&& thread : threads) {
    thread.join();
  }
}

MultiThreadFiberScheduler& MultiThreadFiberScheduler::GetInstance() {
  static MultiThreadFiberScheduler x;
  return x;
}

void MultiThreadFiberScheduler::Schedule(const std::function<void()>& fn) {
  if (this_fiber::get_thread_id() != -1) {
    // FiberEnvironment::GetInstance()->raisedTasks_.push_back(fn);
    mutex.lock();
    commTasks.push_back(fn);
    mutex.unlock();
  } else {
    mutex.lock();
    commTasks.push_back(fn);
    mutex.unlock();
  }
}

void MultiThreadFiberScheduler::Schedule(std::function<void()>&& fn) {
  if (this_fiber::get_thread_id() != -1) [[likely]] {
    // FiberEnvironment::GetInstance()->raisedTasks_.push_back(std::move(fn));
    mutex.lock();
    commTasks.push_back(fn);
    mutex.unlock();
  } else {
    mutex.lock();
    commTasks.push_back(std::move(fn));
    mutex.unlock();
  }
}

}  // namespace pio::fiber

namespace pio::this_fiber {

unsigned long long get_id() { return (unsigned long long)(co_self()); }

int get_thread_id() {
  return fiber::FiberEnvironment::GetInstance()->threadId_;
}

pio::fiber::Fiber* co_self() {
  auto env = fiber::FiberEnvironment::GetInstance();
  return env->pCallStack_[env->callStackSize_ - 1];
}

void sleep_for(const std::chrono::milliseconds& ms) {
  if (co_self()->IsMain()) {
    std::this_thread::sleep_for(ms);
    return;
  }

  auto timeout = fiber::StTimeoutItem();
  timeout.pfnProcess = fiber::OnPollProcess;
  timeout.pArg = co_self();
  auto now = fiber::GetTickMS();
  timeout.ullExpireTime = now + ms.count();
  fiber::FiberEnvironment::GetInstance()->pTimeWheel_->AddTimeout(&timeout,
                                                                  now);
  co_self()->Yield();
}

void sleep_until(
    const std::chrono::high_resolution_clock::time_point& time_point) {
  using namespace std::chrono;
  if (co_self()->IsMain()) {
    std::this_thread::sleep_until(time_point);
    return;
  }

  auto timeout = fiber::StTimeoutItem();
  timeout.pfnProcess = fiber::OnPollProcess;
  timeout.pArg = co_self();
  auto now = fiber::GetTickMS();
  timeout.ullExpireTime =
      duration_cast<milliseconds>(time_point.time_since_epoch()).count();
  if (fiber::FiberEnvironment::GetInstance()->pTimeWheel_->AddTimeout(
          &timeout, now) == 0) {
    co_self()->Yield();
  }
}

void yield() {
  auto self = co_self();
  if (self->IsMain()) {
    std::this_thread::yield();
    return;
  }
  fiber::FiberEnvironment::GetInstance()->userYieldFiberQ_.push_back(self);
  self->Yield();
}

void enable_system_hook() { co_self()->EnableHook(); }

void disable_system_hook() { co_self()->DisableHook(); }

}  // namespace pio::this_fiber

typedef int (*poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);

int co_poll_inner(pollfd fds[], nfds_t nfds, int timeout, poll_pfn_t pollfunc) {
  if (timeout == 0) {
    return pollfunc(fds, nfds, timeout);
  }

  auto env = pio::fiber::FiberEnvironment::GetInstance();
  int epfd = env->EpollFd_;

  // 获取当前协程
  pio::fiber::Fiber* self = pio::this_fiber::co_self();

  // 1.struct change
  pio::fiber::StCoPoller* arg = new pio::fiber::StCoPoller(epfd, nfds);

  pio::fiber::StCoPollItem arr[2];
  if (nfds < 2) {
    arg->pPollItems = arr;
  } else {  // 如果监听的描述符在2个及以上
    arg->pPollItems = new pio::fiber::StCoPollItem[nfds];
  }

  // 当事件到来的时候，就调用这个callback。
  // 这个callback内部做了co_resume的动作
  arg->pfnProcess = pio::fiber::OnPollProcess;
  arg->pArg = pio::this_fiber::co_self();

  // 2. add epoll
  for (nfds_t i = 0; i < nfds; i++) {
    arg->pPollItems[i].pSelf = arg->fds + i;
    arg->pPollItems[i].pPoll = arg;

    // 设置一个预处理的callback
    // 这个函数会在事件active的时候首先触发
    arg->pPollItems[i].pfnPrepare = pio::fiber::OnPollPrepare;
    struct epoll_event& ev = arg->pPollItems[i].stEvent;

    if (fds[i].fd > -1) {
      ev.data.ptr = arg->pPollItems + i;
      ev.events = pio::fiber::StCoPoller::PollEvent2Epoll(fds[i].events);

      int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fds[i].fd, &ev);

      if (ret < 0 && errno == EPERM && nfds == 1 && pollfunc != NULL) {
        if (arg->pPollItems != arr) {
          delete[] arg->pPollItems;
          arg->pPollItems = NULL;
        }
        delete arg;
        return pollfunc(fds, nfds, timeout);
      }
    }
    // if fail,the timeout would work
  }

  // 3.add timeout
  unsigned long long now = pio::fiber::GetTickMS();
  arg->ullExpireTime = timeout > 0 ? (now + timeout) : UINT64_MAX;
  int ret = env->pTimeWheel_->AddTimeout(arg, now);
  int iRaiseCnt = 0;
  if (ret != 0) {
    errno = EINVAL;
    iRaiseCnt = -1;

  } else {
    pio::this_fiber::co_self()->Yield();
    iRaiseCnt = arg->iRaiseCnt;
  }

  // clear epoll status and memory
  arg->RemoveFromLink();
  for (nfds_t i = 0; i < nfds; i++) {
    int fd = fds[i].fd;
    if (fd > -1) {
      epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &arg->pPollItems[i].stEvent);
    }
    fds[i].revents = arg->fds[i].revents;
  }

  if (arg->pPollItems != arr) {
    delete[] arg->pPollItems;
    arg->pPollItems = NULL;
  }

  delete arg;

  return iRaiseCnt;
}

int co_poll(pollfd fds[], nfds_t nfds, int timeout_ms) {
  return co_poll_inner(fds, nfds, timeout_ms, NULL);
}