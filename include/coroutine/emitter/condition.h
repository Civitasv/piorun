#ifndef PIORUN_COROUTINE_EMITTER_CONDITION_H_
#define PIORUN_COROUTINE_EMITTER_CONDITION_H_

#include <unordered_set>

#include "coroutine/awaitable/event.h"
#include "coroutine/emitter/base.h"

namespace pio {
namespace emitter {
/**
 * @brief 监听 awaiting_，判断其中的条件表达式是否为 true
 *        如果为 true，则将该数据返回
 *        若全不为 true，则返回 nullptr
 */
struct Condition : public Base {
  awaitable::Event *Emit() override;

  /**
   * @brief 监听该事件
   *
   * @param data 事件
   */
  void NotifyArrival(awaitable::Event *event) override;

  /**
   * @brief 取消监听该事件
   *
   * @param data 事件
   */
  void NotifyDeparture(awaitable::Event *event) override;
  bool IsEmpty() override;

  virtual ~Condition() {}

 private:
  std::unordered_set<awaitable::Event *> awaiting_;
};

}  // namespace emitter
}  // namespace pio

#endif  // PIORUN_COROUTINE_EMITTER_CONDITION_H_