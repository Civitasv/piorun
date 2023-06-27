#pragma once

#include <atomic>

// 无锁链栈
template <typename T>
class LockFreeStack {
 public:
  LockFreeStack() : list_head(nullptr) {}

  ~LockFreeStack() {
    Node* p = list_head;
    while (p != nullptr) {
      Node* tmp = p->next;
      delete p;
      p = tmp;
    }
    list_head = nullptr;
  }

  bool push(const T& value) {
    Node* p = new Node{value, list_head};
    while (list_head.compare_exchange_weak(p->next, p) == false)
      ;
    return true;
  }

  bool pop(T& value) {
    Node* p = list_head;
    while (p != nullptr && list_head.compare_exchange_weak(p, p->next) == false)
      ;
    if (p == nullptr) return false;
    value = std::move(p->value);
    delete p;
    return true;
  }

  bool empty() const { return list_head == nullptr; }

  // just for debug
  void disp() const {
    for (Node* it = list_head; it != nullptr; it = it->next)
      printf("%d ", it->value);
    printf("\n");
  }

 private:
  struct Node {
    T value;
    Node* next;
  };

  std::atomic<Node*> list_head;
};