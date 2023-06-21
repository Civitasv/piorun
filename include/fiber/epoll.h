/**
 * @file epoll.h
 * @author horse-dog (horsedog@whu.edu.cn)
 * @brief 有栈协程
 * @version 0.1
 * @date 2023-06-12
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef PIORUN_FIBER_EPOLL_H_
#define PIORUN_FIBER_EPOLL_H_

struct co_epoll_res {
  int size;                   /*> epoll_wait返回的事件的数量 */
  struct epoll_event *events; /*> epoll_wait返回的事件的数组 */
};

#endif