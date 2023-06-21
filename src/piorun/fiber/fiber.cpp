#include <fiber/fiber.h>
#include <fiber/inner.h>

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/epoll.h>

namespace pio::fiber {

extern "C" {

extern void coctx_swap(StCoContext*, StCoContext*) asm("coctx_swap");

}

// TODO: move
/**
 * @brief 获取当前系统时间（毫秒级）
 *
 * @return unsigned long long
 */
static unsigned long long GetTickMS() {
#if defined(__LIBCO_RDTSCP__)
  static uint32_t khz = getCpuKhz();
  return counter() / khz;
#else
  // 1秒 = 1000毫秒 = 1000'000微秒 = 1000'000'000纳秒
  // timeval结构体由秒 + 微秒组成
  struct timeval now = {0};
  gettimeofday(&now, NULL);
  unsigned long long u = now.tv_sec;
  u *= 1000;
  u += now.tv_usec / 1000;
  return u;
#endif
}

int Run(StCoRoutine* co, void*) {
  if (co->pfn_ != nullptr) {
    co->pfn_(co->arg_);
  }
  co->cEnd_ = 1;
  co->Yield();
  return 0;
}

static thread_local StCoRoutineEnv* gCoEnvPerThread = nullptr;

struct StTimeoutItemLink;

struct StTimeoutItem {

  using OnPreparePfn_t = void(*)(StTimeoutItem*, epoll_event& ev, StTimeoutItemLink*);
  using OnProcessPfn_t = void(*)(StTimeoutItem*);

  enum
  { eMaxTimeout =  40 * 1000  /*> 最长超时时间: 40s */ };
  StTimeoutItem*       pPrev; /*> 指向上一个结点 */
  StTimeoutItem*       pNext; /*> 指向下一个结点 */
  StTimeoutItemLink*   pLink; /*> 指向本结点所属的链表 */

  uint64_t     ullExpireTime; /*> 超时时间 */

  OnPreparePfn_t  pfnPrepare; /*> 预处理函数，在eventloop中会被调用 */
  OnProcessPfn_t  pfnProcess; /*> 处理函数，在eventloop中会被调用 */

  void*                 pArg; /*> 携带一些信息，例如可以用于指向该超时结点所在的协程 */
  bool              bTimeout; /*> 本结点是否已经超时 */

  StTimeoutItem() 
  : pPrev(), pNext(), pLink(), ullExpireTime(), pfnPrepare(), pfnProcess(), pArg(), bTimeout(false) {}

 ~StTimeoutItem() {}

  void RemoveFromLink();

};

struct StTimeoutItemLink {

  StTimeoutItem*       head_; /*> 指向链表头部 */
  StTimeoutItem*       tail_; /*> 指向链表尾部 */

  StTimeoutItemLink() : head_(), tail_() {}

 ~StTimeoutItemLink() {}

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
    } else {  // 链表还是空的。。。
      this->head_ = this->tail_ = ap;
      ap->pNext = ap->pPrev = NULL;  // 这里说明实现的是无环的双链表
    }
    // 最后记得设置parent
    ap->pLink = this;
  }

  void inline PopHead() {
    if (this->head_ == nullptr)
      return;
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

  void inline Join(StTimeoutItemLink *apOther) {
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

struct StTimeout {

  StTimeoutItemLink* pItems_; /*> 时间轮，是一个由多个超时事件链表组成的数组 */
  int             iItemSize_; /*> 时间轮的大小，默认为60*1000 */

  uint64_t         ullStart_; /*> 上一次取超时事件的绝对时间 */
  int64_t        llStartIdx_; /*> 上一次取超时事件的槽号范围的最大值 +
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

  int AddTimeout(StTimeoutItem *apItem, uint64_t allNow) {
    if (this->ullStart_ == 0) {
      this->ullStart_ = allNow;
      this->llStartIdx_ = 0;
    }

    // 计算时间差，单位为ms
    auto diff = apItem->ullExpireTime - this->ullStart_;

    // 比上一次取超时事件的时间还要长超过了一分钟（60*1000ms)，（溢出了。。。）报错
    // 不过这里又改为了将超时时间设置为最长支持的时间，然后添加。。。
    if (diff >= (uint64_t)this->iItemSize_) {
      diff = this->iItemSize_ - 1;
    }

    // 好的，把这个新结点添加到时间轮中对应的槽指向的链表的尾部
    this->pItems_[(llStartIdx_ + diff) % iItemSize_].AddTail(apItem);
    return 0;
  }

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

struct StCoEpoller {

  struct Result {
    int                size; /*> epoll_wait返回的事件的数量 */
    epoll_event*     events; /*> epoll_wait返回的事件的数组 */

    Result(int sz) : size(sz) { events = new epoll_event[sz]; }
   ~Result() { delete[] events; }
  };

  int                      iEpollFd_; /*> epfd */
  StTimeout*               pTimeout_; /*> 超时管理器 */
  StTimeoutItemLink  pstTimeoutList_; /*> 用于管理超时事件的链表 */
  StTimeoutItemLink   pstActiveList_; /*> 用于管理未超时且到达的事件的链表 */
  Result*                    result_; /*> 存储结果 */

  /*> epoll_wait最大支持的事件数 */
  static const int EPOLL_SIZE_ = 1024 * 10;

  StCoEpoller() 
  : pTimeout_(), pstTimeoutList_(), pstActiveList_(), result_() {
    iEpollFd_ = epoll_create(EPOLL_SIZE_);
    pTimeout_ = new StTimeout(60 * 1000);
    result_   = new Result(EPOLL_SIZE_);
  }

 ~StCoEpoller() {
    delete pTimeout_;
    delete result_;
    pTimeout_ = nullptr;
    result_ = nullptr;
    iEpollFd_ = -1;
  }

  int EpollCtrl(int op, int fd, epoll_event* ev) { return epoll_ctl(iEpollFd_, op, fd, ev); }

  int EpollWait(int timeout) { return epoll_wait(iEpollFd_, result_->events, result_->size, timeout); }

};

struct StCoPollItem;

struct StCoPoller : public StTimeoutItem {

  pollfd*              fds; /*> pollfd数组，用于注册和获取事件，每个元素与一个Poll结点相对应 */
  nfds_t              nfds; /*> fds数组的长度 */
  StCoPollItem* pPollItems; /* 该管理者所管理的Poll结点所组成的数组 */
  int      iAllEventDetach; /*> 该Poll结构体等待的事件数组中，只有有一个事件到达了，该标识设置为1 */
  int             iEpollFd; /*> epfd */
  int            iRaiseCnt; /*> poll的active事件个数 */

  StCoPoller(int epfd, nfds_t numfds) 
  : fds(), nfds(numfds), pPollItems(), iAllEventDetach(), iEpollFd(epfd), iRaiseCnt() {
    fds = new pollfd[numfds];
  }

  ~StCoPoller() {
    delete[] fds;
  }

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

  pollfd*       pSelf; /*> 指向该Poll结点对应的pollfd */
  StCoPoller*   pPoll; /*> 指向管理该Poll结点的Poll结构体 */
  epoll_event stEvent; /*> 用于存放关心的epoll事件 */

  StCoPollItem() : pSelf(), pPoll(), stEvent() {}

};

static void OnPollProcess(StTimeoutItem *ap) {
  StCoRoutine *co = (StCoRoutine*)ap->pArg;
  co->Resume();
}

static void OnPollPrepare(StTimeoutItem *ap, epoll_event &e, StTimeoutItemLink *active) {
  StCoPollItem *lp = (StCoPollItem*) ap;
  lp->pSelf->revents = StCoPoller::EpollEvent2Poll(e.events);

  StCoPoller *pPoll = lp->pPoll;
  pPoll->iRaiseCnt++;

  if (!pPoll->iAllEventDetach) {
    pPoll->iAllEventDetach = 1;
    pPoll->RemoveFromLink();
    active->AddTail(pPoll);
  }
}

struct StCoRoutineEnv {

  // 这里实际上维护的是个调用栈
  // 最后一位是当前运行的协程，前一位是当前协程的父协程(即，resume该协程的协程)
  StCoRoutine* pCallStack_[128];
  int            callStackSize_; // 当前调用栈长度
  StCoEpoller*        pEpoller_; // 主要是epoll，作为协程的调度器

  StCoRoutineEnv() : pCallStack_(), callStackSize_(), pEpoller_() {
    StCoRoutine *self = new StCoRoutine(nullptr, nullptr);
    self->cIsMain_ = 1;
    // 入栈主协程
    pCallStack_[callStackSize_++] = self;
    pEpoller_ = new StCoEpoller();
  }

  ~StCoRoutineEnv() {
    StCoRoutine* mainCo = pCallStack_[0];
    delete mainCo;
    delete pEpoller_;
    pEpoller_ = nullptr;
    callStackSize_ = 0;
  }

};

StCoRoutine::StCoRoutine(pfn_co_routine_t pfn, void* arg)
: pfn_(pfn), arg_(arg), cStart_(0), cEnd_(0), cIsMain_(0),
  cEnableSysHook_(1), stack_mem_(nullptr), stack_sp_(nullptr) {
  // 确保本线程协程环境已经初始化
  if (CoGetCurrThreadEnv() == nullptr) {
    CoInitCurrThreadEnv();
  }

  this->env_ = gCoEnvPerThread;
  stack_mem_ = new StStackMem();
  this->ctx_.sp_ = stack_mem_->buffer_;
  this->ctx_.size_ = stack_mem_->stackSize_;
  this->ctx_.Make((StCoContext::StCoContextFunc)(Run), this, nullptr);
}

StCoRoutine::~StCoRoutine() {
  delete stack_mem_;
  stack_mem_ = nullptr;
  stack_sp_ = nullptr;
  cStart_ = cEnd_ = 0;
}

StCoRoutineEnv* StCoRoutine::CoGetCurrThreadEnv() const {
  return gCoEnvPerThread;
}

void StCoRoutine::CoInitCurrThreadEnv() {
  gCoEnvPerThread = (StCoRoutineEnv*)(1);
  gCoEnvPerThread = new StCoRoutineEnv();
}

void StCoRoutine::Yield() {
  StCoRoutine* last = env_->pCallStack_[env_->callStackSize_ - 2];
  env_->callStackSize_--;
  SwapContext(last);
}

void StCoRoutine::Resume() {
  StCoRoutine* curr = env_->pCallStack_[env_->callStackSize_ - 1];
  if (!this->cStart_) {
    this->cStart_ = 1;
  }
  env_->pCallStack_[env_->callStackSize_++] = this;
  curr->SwapContext(this);
}

void StCoRoutine::Reset() {
  if (!cStart_ || cIsMain_) return;
  cStart_ = 0;
  cEnd_   = 0;
}

bool StCoRoutine::Done() const {
  return cEnd_ != 0;
}

void StCoRoutine::SwapContext(StCoRoutine* pendingCo) {
  char c;
  this->stack_sp_ = &c;
  coctx_swap(&(this->ctx_), &(pendingCo->ctx_));
}

void co_eventloop(pfn_co_eventloop_t pfn, void *arg) {

  auto epoller = gCoEnvPerThread->pEpoller_;
  auto tmWheel = epoller->pTimeout_;
  auto result  = epoller->result_;
  
  while (true) {
    int eventNum = epoller->EpollWait(1); // wait for 1 second
    auto  active = epoller->pstActiveList_;
    auto timeout = epoller->pstTimeoutList_;

    timeout.head_ = timeout.tail_ = nullptr;

    for (int i = 0; i < eventNum; i++) {
      // get the timeoutNode of events[i].
      auto timeoutNode = (StTimeoutItem*)result->events[i].data.ptr;
      if (timeoutNode->pfnPrepare) {
        timeoutNode->pfnPrepare(timeoutNode, result->events[i], &active);
      } else {
        active.AddTail(timeoutNode);
      }
    }

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
        if (!ret) {
          lp->bTimeout = false;
          lp = active.head_;
          continue;
        }
      }
      // deal callback.
      if (lp->pfnProcess) lp->pfnProcess(lp);
      lp = active.head_;
    }

    if (pfn != nullptr) {
      if (pfn(arg) == -1) {
        break;
      }
    }
  }
}

typedef int (*poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);

int co_poll_inner(StCoEpoller *ctx, pollfd fds[], nfds_t nfds, int timeout, poll_pfn_t pollfunc) {
  if (timeout == 0) {
    return pollfunc(fds, nfds, timeout);
  }

  if (timeout < 0) {
    timeout = INT32_MAX;
  }

  int epfd = ctx->iEpollFd_;

  // 获取当前协程
  StCoRoutine *self = this_coroutine::co_self();

  // 1.struct change
  StCoPoller* arg = new StCoPoller(epfd, nfds);

  StCoPollItem arr[2];
  if (nfds < 2) {
    arg->pPollItems = arr;
  } else {  // 如果监听的描述符在2个以上，或者协程本身采用共享栈
    arg->pPollItems = new StCoPollItem[nfds];
  }

  // 当事件到来的时候，就调用这个callback。
  // 这个callback内部做了co_resume的动作
  arg->pfnProcess = OnPollProcess;
  arg->pArg = this_coroutine::co_self();

  // 2. add epoll
  for (nfds_t i = 0; i < nfds; i++) {
    arg->pPollItems[i].pSelf = arg->fds + i;
    arg->pPollItems[i].pPoll = arg;

    // 设置一个预处理的callback
    // 这个函数会在事件active的时候首先触发
    arg->pPollItems[i].pfnPrepare = OnPollPrepare;
    struct epoll_event &ev = arg->pPollItems[i].stEvent;

    if (fds[i].fd > -1) {
      ev.data.ptr = arg->pPollItems + i;
      ev.events = StCoPoller::PollEvent2Epoll(fds[i].events);

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
  unsigned long long now = GetTickMS();
  arg->ullExpireTime = now + timeout;
  int ret = ctx->pTimeout_->AddTimeout(arg, now);
  int iRaiseCnt = 0;
  if (ret != 0) {
    // co_log_err(
    //     "CO_ERR: AddTimeout ret %d now %lld timeout %d arg.ullExpireTime %lld",
    //     ret, now, timeout, arg.ullExpireTime);
    errno = EINVAL;
    iRaiseCnt = -1;

  } else {
    this_coroutine::yield();
    // co_yield_env(co_get_curr_thread_env());
    iRaiseCnt = arg->iRaiseCnt;
  }

  {
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
  }

  return iRaiseCnt;
}

int co_poll(StCoEpoller *ctx, pollfd fds[], nfds_t nfds, int timeout_ms) {
  return co_poll_inner(ctx, fds, nfds, timeout_ms, NULL);
}

__attribute__((destructor)) void cleanup() {
  delete gCoEnvPerThread;
}

}

namespace pio::this_coroutine {

unsigned long long get_id() {
  return (unsigned long long)(co_self());
}

pio::fiber::StCoRoutine* co_self() {
  auto env = pio::fiber::gCoEnvPerThread;
  return env->pCallStack_[env->callStackSize_ - 1];
}

void yield() {
  auto self = co_self();
  self->Yield();
}

void sleep_for(const std::chrono::milliseconds& ms) {
  auto timeout = new fiber::StTimeoutItem();
  timeout->pfnProcess = fiber::OnPollProcess;
  timeout->pArg = co_self();
  auto now = fiber::GetTickMS();
  timeout->ullExpireTime = now + ms.count();
  fiber::gCoEnvPerThread->pEpoller_->pTimeout_->AddTimeout(timeout, now);
  yield();
}

}