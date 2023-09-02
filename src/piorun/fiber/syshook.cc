#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <resolv.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <map>

#include "fiber/fiber.h"

struct rpchook_t {
  int user_flag;            // user flag in fcntl
  struct sockaddr_in dest;  // maybe sockaddr_un;
  int domain;               // AF_UNIX , AF_INET

  struct timeval read_timeout;
  struct timeval write_timeout;
};

class FdContextManager {
 private:
  FdContextManager(size_t sz = 102400) : fdContexts_{sz, nullptr} {}

  FdContextManager(const FdContextManager &) = delete;

  FdContextManager(FdContextManager &&) = delete;

  ~FdContextManager() {}

 public:
  static FdContextManager &GetInstance() {
    static FdContextManager x;
    return x;
  }

  rpchook_t *GetContextByFd(int fd) {
    rwMutex_.lock_shared();
    if (fd < fdContexts_.size() && fdContexts_[fd] != nullptr) {
      rpchook_t *result = fdContexts_[fd];
      rwMutex_.unlock_shared();
      return result;
    }
    rwMutex_.unlock_shared();

    rwMutex_.lock();
    if (fdContexts_.size() <= fd) [[unlikely]] {
      fdContexts_.resize(static_cast<size_t>(fd) * 3 / 2, nullptr);
    }

    rpchook_t *result = new rpchook_t{0};
    fdContexts_[fd] = result;
    fdContexts_[fd]->read_timeout.tv_sec = -1;
    fdContexts_[fd]->write_timeout.tv_sec = -1;
    rwMutex_.unlock();
    return result;
  }

  void DelContextByFd(int fd) {
    rwMutex_.lock();
    if (fdContexts_.size() <= fd) {
      rwMutex_.unlock();
      return;
    }
    delete fdContexts_[fd];
    fdContexts_[fd] = nullptr;
    rwMutex_.unlock();
  }

 private:
  std::deque<rpchook_t *> fdContexts_;
  std::shared_mutex rwMutex_;
};

typedef int (*socket_pfn_t)(int domain, int type, int protocol);
typedef int (*accept_pfn_t)(int fd, sockaddr *addr, socklen_t *len);
typedef int (*connect_pfn_t)(int socket, const struct sockaddr *address,
                             socklen_t address_len);
typedef int (*close_pfn_t)(int fd);

typedef ssize_t (*read_pfn_t)(int fildes, void *buf, size_t nbyte);
typedef ssize_t (*write_pfn_t)(int fildes, const void *buf, size_t nbyte);

typedef ssize_t (*sendto_pfn_t)(int socket, const void *message, size_t length,
                                int flags, const struct sockaddr *dest_addr,
                                socklen_t dest_len);

typedef ssize_t (*recvfrom_pfn_t)(int socket, void *buffer, size_t length,
                                  int flags, struct sockaddr *address,
                                  socklen_t *address_len);

typedef ssize_t (*send_pfn_t)(int socket, const void *buffer, size_t length,
                              int flags);
typedef ssize_t (*recv_pfn_t)(int socket, void *buffer, size_t length,
                              int flags);

typedef int (*poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);
typedef int (*setsockopt_pfn_t)(int socket, int level, int option_name,
                                const void *option_value, socklen_t option_len);

typedef int (*fcntl_pfn_t)(int fildes, int cmd, ...);
typedef struct tm *(*localtime_r_pfn_t)(const time_t *timep, struct tm *result);

typedef void *(*pthread_getspecific_pfn_t)(pthread_key_t key);
typedef int (*pthread_setspecific_pfn_t)(pthread_key_t key, const void *value);

typedef int (*setenv_pfn_t)(const char *name, const char *value, int overwrite);
typedef int (*unsetenv_pfn_t)(const char *name);
typedef char *(*getenv_pfn_t)(const char *name);
typedef hostent *(*gethostbyname_pfn_t)(const char *name);
typedef res_state (*__res_state_pfn_t)();
typedef int (*__poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);
typedef int (*gethostbyname_r_pfn_t)(const char *__restrict name,
                                     struct hostent *__restrict __result_buf,
                                     char *__restrict __buf, size_t __buflen,
                                     struct hostent **__restrict __result,
                                     int *__restrict __h_errnop);

static socket_pfn_t g_sys_socket_func =
    (socket_pfn_t)dlsym(RTLD_NEXT, "socket");
static accept_pfn_t g_sys_accept_func =
    (accept_pfn_t)dlsym(RTLD_NEXT, "accept");
static connect_pfn_t g_sys_connect_func =
    (connect_pfn_t)dlsym(RTLD_NEXT, "connect");
static close_pfn_t g_sys_close_func = (close_pfn_t)dlsym(RTLD_NEXT, "close");

static read_pfn_t g_sys_read_func = (read_pfn_t)dlsym(RTLD_NEXT, "read");
static write_pfn_t g_sys_write_func = (write_pfn_t)dlsym(RTLD_NEXT, "write");

static sendto_pfn_t g_sys_sendto_func =
    (sendto_pfn_t)dlsym(RTLD_NEXT, "sendto");
static recvfrom_pfn_t g_sys_recvfrom_func =
    (recvfrom_pfn_t)dlsym(RTLD_NEXT, "recvfrom");

static send_pfn_t g_sys_send_func = (send_pfn_t)dlsym(RTLD_NEXT, "send");
static recv_pfn_t g_sys_recv_func = (recv_pfn_t)dlsym(RTLD_NEXT, "recv");

static poll_pfn_t g_sys_poll_func = (poll_pfn_t)dlsym(RTLD_NEXT, "poll");

static setsockopt_pfn_t g_sys_setsockopt_func =
    (setsockopt_pfn_t)dlsym(RTLD_NEXT, "setsockopt");
static fcntl_pfn_t g_sys_fcntl_func = (fcntl_pfn_t)dlsym(RTLD_NEXT, "fcntl");

static setenv_pfn_t g_sys_setenv_func =
    (setenv_pfn_t)dlsym(RTLD_NEXT, "setenv");
static unsetenv_pfn_t g_sys_unsetenv_func =
    (unsetenv_pfn_t)dlsym(RTLD_NEXT, "unsetenv");
static getenv_pfn_t g_sys_getenv_func =
    (getenv_pfn_t)dlsym(RTLD_NEXT, "getenv");
static __res_state_pfn_t g_sys___res_state_func =
    (__res_state_pfn_t)dlsym(RTLD_NEXT, "__res_state");

static gethostbyname_pfn_t g_sys_gethostbyname_func =
    (gethostbyname_pfn_t)dlsym(RTLD_NEXT, "gethostbyname");
static gethostbyname_r_pfn_t g_sys_gethostbyname_r_func =
    (gethostbyname_r_pfn_t)dlsym(RTLD_NEXT, "gethostbyname_r");

static __poll_pfn_t g_sys___poll_func =
    (__poll_pfn_t)dlsym(RTLD_NEXT, "__poll");

#define HOOK_SYS_FUNC(name)                                      \
  if (!g_sys_##name##_func) {                                    \
    g_sys_##name##_func = (name##_pfn_t)dlsym(RTLD_NEXT, #name); \
  }

int socket(int domain, int type, int protocol) {
  HOOK_SYS_FUNC(socket);
  if (!pio::this_fiber::co_self()->IsHooked()) {
    return g_sys_socket_func(domain, type, protocol);
  }
  int fd = g_sys_socket_func(domain, type, protocol);
  if (fd < 0) {
    return fd;
  }

  rpchook_t *lp = FdContextManager::GetInstance().GetContextByFd(fd);
  lp->domain = domain;

  fcntl(fd, F_SETFL, g_sys_fcntl_func(fd, F_GETFL));

  return fd;
}

extern thread_local bool isaccept;

int accept(int fd, struct sockaddr *addr, socklen_t *len) {
  if (!g_sys_accept_func) { 
    g_sys_accept_func = (accept_pfn_t)dlsym(((void *) -1l), "accept");
    isaccept = true;
  }
  if (!pio::this_fiber::co_self()->IsHooked()) {
    return g_sys_accept_func(fd, addr, len);
  }
  rpchook_t *lp = FdContextManager::GetInstance().GetContextByFd(fd);
  if (!lp || (O_NONBLOCK & lp->user_flag)) {
    int cli = g_sys_accept_func(fd, addr, len);
    if (cli >= 0) {
      fcntl(cli, F_SETFL, g_sys_fcntl_func(cli, F_GETFL));
    }
    return cli;
  }

  int timeout = lp->read_timeout.tv_sec == -1
                    ? -1
                    : (lp->read_timeout.tv_sec * 1000) +
                          (lp->read_timeout.tv_usec / 1000);
  struct pollfd pf = {0};
  pf.fd = fd;
  pf.events = (POLLIN | POLLERR | POLLHUP);
  int pollret = poll(&pf, 1, timeout);
  int cli = g_sys_accept_func(fd, addr, len);
  if (cli >= 0) {
    fcntl(cli, F_SETFL, g_sys_fcntl_func(cli, F_GETFL));
  }
  return cli;
}

int connect(int fd, const struct sockaddr *address, socklen_t address_len) {
  HOOK_SYS_FUNC(connect);
  if (!pio::this_fiber::co_self()->IsHooked()) {
    return g_sys_connect_func(fd, address, address_len);
  }

  // 1.sys call
  int ret = g_sys_connect_func(fd, address, address_len);

  rpchook_t *lp = FdContextManager::GetInstance().GetContextByFd(fd);
  if (!lp) return ret;

  if (sizeof(lp->dest) >= address_len) {
    memcpy(&(lp->dest), address, (int)address_len);
  }

  if (O_NONBLOCK & lp->user_flag) {
    return ret;
  }

  if (!(ret < 0 && errno == EINPROGRESS)) {
    return ret;
  }

  // 2.wait
  int pollret = 0;
  struct pollfd pf = {0};

  for (int i = 0; i < 3; i++)  // 25s * 3 = 75s
  {
    memset(&pf, 0, sizeof(pf));
    pf.fd = fd;
    pf.events = (POLLOUT | POLLERR | POLLHUP);

    pollret = poll(&pf, 1, 25000);

    if (1 == pollret) {
      break;
    }
  }

  if (pf.revents & POLLOUT)  // connect succ
  {
    // 3.check getsockopt ret
    int err = 0;
    socklen_t errlen = sizeof(err);
    ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    if (ret < 0) {
      return ret;
    } else if (err != 0) {
      errno = err;
      return -1;
    }
    errno = 0;
    return 0;
  }

  errno = ETIMEDOUT;
  return ret;
}

int close(int fd) {
  HOOK_SYS_FUNC(close);

  if (!pio::this_fiber::co_self()->IsHooked()) {
    return g_sys_close_func(fd);
  }

  FdContextManager::GetInstance().DelContextByFd(fd);
  int ret = g_sys_close_func(fd);

  return ret;
}

// ssize_t readv(int __fd, const iovec *__iovec, int __count) {}

ssize_t read(int fd, void *buf, size_t nbyte) {
  HOOK_SYS_FUNC(read);

  if (!pio::this_fiber::co_self()->IsHooked()) {
    return g_sys_read_func(fd, buf, nbyte);
  }
  rpchook_t *lp = FdContextManager::GetInstance().GetContextByFd(fd);

  if (!lp || (O_NONBLOCK & lp->user_flag)) {
    ssize_t ret = g_sys_read_func(fd, buf, nbyte);
    return ret;
  }
  int timeout = lp->read_timeout.tv_sec == -1
                    ? -1
                    : (lp->read_timeout.tv_sec * 1000) +
                          (lp->read_timeout.tv_usec / 1000);

  struct pollfd pf = {0};
  pf.fd = fd;
  pf.events = (POLLIN | POLLERR | POLLHUP);

  int pollret = poll(&pf, 1, timeout);

  ssize_t readret = g_sys_read_func(fd, (char *)buf, nbyte);

  return readret;
}

ssize_t write(int fd, const void *buf, size_t nbyte) {
  HOOK_SYS_FUNC(write);
  // printf("hooking write\n");

  if (!pio::this_fiber::co_self()->IsHooked()) {
    return g_sys_write_func(fd, buf, nbyte);
  }
  rpchook_t *lp = FdContextManager::GetInstance().GetContextByFd(fd);

  if (!lp || (O_NONBLOCK & lp->user_flag)) {
    ssize_t ret = g_sys_write_func(fd, buf, nbyte);
    return ret;
  }
  size_t wrotelen = 0;
  int timeout = lp->write_timeout.tv_sec == -1
                    ? -1
                    : (lp->write_timeout.tv_sec * 1000) +
                          (lp->write_timeout.tv_usec / 1000);

  ssize_t writeret =
      g_sys_write_func(fd, (const char *)buf + wrotelen, nbyte - wrotelen);

  if (writeret == 0) {
    return writeret;
  }

  if (writeret > 0) {
    wrotelen += writeret;
  }
  while (wrotelen < nbyte) {
    struct pollfd pf = {0};
    pf.fd = fd;
    pf.events = (POLLOUT | POLLERR | POLLHUP);
    poll(&pf, 1, timeout);

    writeret =
        g_sys_write_func(fd, (const char *)buf + wrotelen, nbyte - wrotelen);

    if (writeret <= 0) {
      break;
    }
    wrotelen += writeret;
  }
  if (writeret <= 0 && wrotelen == 0) {
    return writeret;
  }
  return wrotelen;
}

ssize_t sendto(int socket, const void *message, size_t length, int flags,
               const struct sockaddr *dest_addr, socklen_t dest_len) {
  /*
          1.no enable sys call ? sys
          2.( !lp || lp is non block ) ? sys
          3.try
          4.wait
          5.try
  */
  HOOK_SYS_FUNC(sendto);
  if (!pio::this_fiber::co_self()->IsHooked()) {
    return g_sys_sendto_func(socket, message, length, flags, dest_addr,
                             dest_len);
  }

  rpchook_t *lp = FdContextManager::GetInstance().GetContextByFd(socket);
  if (!lp || (O_NONBLOCK & lp->user_flag)) {
    return g_sys_sendto_func(socket, message, length, flags, dest_addr,
                             dest_len);
  }

  ssize_t ret =
      g_sys_sendto_func(socket, message, length, flags, dest_addr, dest_len);
  if (ret < 0 && EAGAIN == errno) {
    int timeout = lp->write_timeout.tv_sec == -1
                      ? -1
                      : (lp->write_timeout.tv_sec * 1000) +
                            (lp->write_timeout.tv_usec / 1000);

    struct pollfd pf = {0};
    pf.fd = socket;
    pf.events = (POLLOUT | POLLERR | POLLHUP);
    poll(&pf, 1, timeout);

    ret =
        g_sys_sendto_func(socket, message, length, flags, dest_addr, dest_len);
  }
  return ret;
}

ssize_t recvfrom(int socket, void *buffer, size_t length, int flags,
                 struct sockaddr *address, socklen_t *address_len) {
  HOOK_SYS_FUNC(recvfrom);
  if (!pio::this_fiber::co_self()->IsHooked()) {
    return g_sys_recvfrom_func(socket, buffer, length, flags, address,
                               address_len);
  }

  rpchook_t *lp = FdContextManager::GetInstance().GetContextByFd(socket);
  if (!lp || (O_NONBLOCK & lp->user_flag)) {
    return g_sys_recvfrom_func(socket, buffer, length, flags, address,
                               address_len);
  }

  int timeout = lp->read_timeout.tv_sec == -1
                    ? -1
                    : (lp->read_timeout.tv_sec * 1000) +
                          (lp->read_timeout.tv_usec / 1000);

  struct pollfd pf = {0};
  pf.fd = socket;
  pf.events = (POLLIN | POLLERR | POLLHUP);
  poll(&pf, 1, timeout);

  ssize_t ret =
      g_sys_recvfrom_func(socket, buffer, length, flags, address, address_len);
  return ret;
}

ssize_t send(int socket, const void *buffer, size_t length, int flags) {
  HOOK_SYS_FUNC(send);

  if (!pio::this_fiber::co_self()->IsHooked()) {
    return g_sys_send_func(socket, buffer, length, flags);
  }
  rpchook_t *lp = FdContextManager::GetInstance().GetContextByFd(socket);

  if (!lp || (O_NONBLOCK & lp->user_flag)) {
    return g_sys_send_func(socket, buffer, length, flags);
  }
  size_t wrotelen = 0;
  int timeout = lp->write_timeout.tv_sec == -1
                    ? -1
                    : (lp->write_timeout.tv_sec * 1000) +
                          (lp->write_timeout.tv_usec / 1000);

  ssize_t writeret = g_sys_send_func(socket, buffer, length, flags);
  if (writeret == 0) {
    return writeret;
  }

  if (writeret > 0) {
    wrotelen += writeret;
  }
  while (wrotelen < length) {
    struct pollfd pf = {0};
    pf.fd = socket;
    pf.events = (POLLOUT | POLLERR | POLLHUP);
    poll(&pf, 1, timeout);

    writeret = g_sys_send_func(socket, (const char *)buffer + wrotelen,
                               length - wrotelen, flags);

    if (writeret <= 0) {
      break;
    }
    wrotelen += writeret;
  }
  if (writeret <= 0 && wrotelen == 0) {
    return writeret;
  }
  return wrotelen;
}

ssize_t recv(int socket, void *buffer, size_t length, int flags) {
  HOOK_SYS_FUNC(recv);

  if (!pio::this_fiber::co_self()->IsHooked()) {
    return g_sys_recv_func(socket, buffer, length, flags);
  }
  rpchook_t *lp = FdContextManager::GetInstance().GetContextByFd(socket);

  if (!lp || (O_NONBLOCK & lp->user_flag)) {
    return g_sys_recv_func(socket, buffer, length, flags);
  }
  int timeout = lp->read_timeout.tv_sec == -1
                    ? -1
                    : (lp->read_timeout.tv_sec * 1000) +
                          (lp->read_timeout.tv_usec / 1000);

  struct pollfd pf = {0};
  pf.fd = socket;
  pf.events = (POLLIN | POLLERR | POLLHUP);

  int pollret = poll(&pf, 1, timeout);

  ssize_t readret = g_sys_recv_func(socket, buffer, length, flags);

  return readret;
}

extern int co_poll_inner(struct pollfd fds[], nfds_t nfds, int timeout,
                         poll_pfn_t pollfunc);

int poll(struct pollfd fds[], nfds_t nfds, int timeout) {
  HOOK_SYS_FUNC(poll);

  if (!pio::this_fiber::co_self()->IsHooked() || timeout == 0) {
    return g_sys_poll_func(fds, nfds, timeout);
  }
  pollfd *fds_merge = NULL;
  nfds_t nfds_merge = 0;
  std::map<int, int> m;  // fd --> idx
  std::map<int, int>::iterator it;
  if (nfds > 1) {
    fds_merge = (pollfd *)malloc(sizeof(pollfd) * nfds);
    for (size_t i = 0; i < nfds; i++) {
      if ((it = m.find(fds[i].fd)) == m.end()) {
        fds_merge[nfds_merge] = fds[i];
        m[fds[i].fd] = nfds_merge;
        nfds_merge++;
      } else {
        int j = it->second;
        fds_merge[j].events |= fds[i].events;  // merge in j slot
      }
    }
  }

  int ret = 0;
  if (nfds_merge == nfds || nfds == 1) {
    ret = co_poll_inner(fds, nfds, timeout, g_sys_poll_func);
  } else {
    ret = co_poll_inner(fds_merge, nfds_merge, timeout, g_sys_poll_func);
    if (ret > 0) {
      for (size_t i = 0; i < nfds; i++) {
        it = m.find(fds[i].fd);
        if (it != m.end()) {
          int j = it->second;
          fds[i].revents = fds_merge[j].revents & fds[i].events;
        }
      }
    }
  }
  free(fds_merge);
  return ret;
}

int setsockopt(int fd, int level, int option_name, const void *option_value,
               socklen_t option_len) {
  HOOK_SYS_FUNC(setsockopt);

  if (!pio::this_fiber::co_self()->IsHooked()) {
    return g_sys_setsockopt_func(fd, level, option_name, option_value,
                                 option_len);
  }
  rpchook_t *lp = FdContextManager::GetInstance().GetContextByFd(fd);

  if (lp && SOL_SOCKET == level) {
    struct timeval *val = (struct timeval *)option_value;
    if (SO_RCVTIMEO == option_name) {
      memcpy(&lp->read_timeout, val, sizeof(*val));
    } else if (SO_SNDTIMEO == option_name) {
      memcpy(&lp->write_timeout, val, sizeof(*val));
    }
  }
  return g_sys_setsockopt_func(fd, level, option_name, option_value,
                               option_len);
}

int fcntl(int fildes, int cmd, ...) {
  HOOK_SYS_FUNC(fcntl);

  if (fildes < 0) {
    return __LINE__;
  }

  va_list arg_list;
  va_start(arg_list, cmd);

  int ret = -1;
  rpchook_t *lp = FdContextManager::GetInstance().GetContextByFd(fildes);
  switch (cmd) {
    case F_DUPFD: {
      int param = va_arg(arg_list, int);
      ret = g_sys_fcntl_func(fildes, cmd, param);
      break;
    }
    case F_GETFD: {
      ret = g_sys_fcntl_func(fildes, cmd);
      break;
    }
    case F_SETFD: {
      int param = va_arg(arg_list, int);
      ret = g_sys_fcntl_func(fildes, cmd, param);
      break;
    }
    case F_GETFL: {
      ret = g_sys_fcntl_func(fildes, cmd);
      if (lp && !(lp->user_flag & O_NONBLOCK)) {
        ret = ret & (~O_NONBLOCK);
      }
      break;
    }
    case F_SETFL: {
      int param = va_arg(arg_list, int);
      int flag = param;
      if (pio::this_fiber::co_self()->IsHooked() && lp) {
        flag |= O_NONBLOCK;
      }
      ret = g_sys_fcntl_func(fildes, cmd, flag);
      if (0 == ret && lp) {
        lp->user_flag = param;
      }
      break;
    }
    case F_GETOWN: {
      ret = g_sys_fcntl_func(fildes, cmd);
      break;
    }
    case F_SETOWN: {
      int param = va_arg(arg_list, int);
      ret = g_sys_fcntl_func(fildes, cmd, param);
      break;
    }
    case F_GETLK: {
      struct flock *param = va_arg(arg_list, struct flock *);
      ret = g_sys_fcntl_func(fildes, cmd, param);
      break;
    }
    case F_SETLK: {
      struct flock *param = va_arg(arg_list, struct flock *);
      ret = g_sys_fcntl_func(fildes, cmd, param);
      break;
    }
    case F_SETLKW: {
      struct flock *param = va_arg(arg_list, struct flock *);
      ret = g_sys_fcntl_func(fildes, cmd, param);
      break;
    }
  }

  va_end(arg_list);

  return ret;
}

// struct stCoSysEnv_t {
//   char *name;
//   char *value;
// };

// struct stCoSysEnvArr_t {
//   stCoSysEnv_t *data;
//   size_t cnt;
// };

// static stCoSysEnvArr_t g_co_sysenv = {0};

// int setenv(const char *n, const char *value, int overwrite) {
//   HOOK_SYS_FUNC(setenv)
//   if (pio::this_fiber::co_self()->IsHooked() && g_co_sysenv.data) {
//     auto *self = pio::this_fiber::co_self();
//     if (self) {
//       if (!self->pvEnv) {
//         self->pvEnv = dup_co_sysenv_arr(&g_co_sysenv);
//       }
//       stCoSysEnvArr_t *arr = (stCoSysEnvArr_t *)(self->pvEnv);

//       stCoSysEnv_t name = {(char *)n, 0};

//       stCoSysEnv_t *e = (stCoSysEnv_t *)bsearch(&name, arr->data, arr->cnt,
//                                                 sizeof(name),
//                                                 co_sysenv_comp);

//       if (e) {
//         if (overwrite || !e->value) {
//           if (e->value) free(e->value);
//           e->value = (value ? strdup(value) : 0);
//         }
//         return 0;
//       }
//     }
//   }
//   return g_sys_setenv_func(n, value, overwrite);
// }

// struct hostent *co_gethostbyname(const char *name);

// struct hostent *gethostbyname(const char *name) {
//   HOOK_SYS_FUNC(gethostbyname);

// #if defined(__APPLE__) || defined(__FreeBSD__)
//   return g_sys_gethostbyname_func(name);
// #else
//   if (!pio::this_fiber::co_self()->IsHooked()) {
//     return g_sys_gethostbyname_func(name);
//   }
//   return co_gethostbyname(name);
// #endif
// }

// int co_gethostbyname_r(const char *__restrict name,
//                        struct hostent *__restrict __result_buf,
//                        char *__restrict __buf, size_t __buflen,
//                        struct hostent **__restrict __result,
//                        int *__restrict __h_errnop) {
//   static __thread clsCoMutex *tls_leaky_dns_lock = NULL;
//   if (tls_leaky_dns_lock == NULL) {
//     tls_leaky_dns_lock = new clsCoMutex();
//   }
//   clsSmartLock auto_lock(tls_leaky_dns_lock);
//   return g_sys_gethostbyname_r_func(name, __result_buf, __buf, __buflen,
//                                     __result, __h_errnop);
// }