#ifndef CSERVER_NET_INCLUDE_TIMER_
#define CSERVER_NET_INCLUDE_TIMER_

#include "noncopyable.h"
#include "Callbacks.h"
#include "Timestamp.h"
#include <atomic>

namespace cServer {

// 定时器事件的内部类。
class Timer : noncopyable {
 public:
  // 构造函数：初始化定时器，传入回调函数、到期时间、以及可选的重复间隔。
  Timer(const TimerCallback &cb, Timestamp when, double interval) : 
        callback_(cb), expiration_(when), interval_(interval), repeat_(interval > 0.0),
        sequence_(s_numCreated_.fetch_add(1)) {
  }

  // 执行定时器的回调函数
  void run() const {
    callback_();
  }

  // 获取定时器的到期时间
  Timestamp expiration() const {
    return expiration_;
  }

  // 检查定时器是否是重复定时器
  bool repeat() const {
    return repeat_;
  }

  // 返回当前Timer对象的序列号
  int64_t sequence() const {
    return sequence_;
  }

  // 重新启动定时器，更新到期时间
  void restart(Timestamp now);

 private:
  const TimerCallback callback_;    // 定时器回调函数
  Timestamp expiration_;            // 定时器到期时间
  const double interval_;           // 定时器的重复间隔，若为负值表示非重复定时器
  const bool repeat_;               // 表示定时器是否是重复定时器
  const int64_t sequence_;          // 当前Timer对象的序列号

  static std::atomic_int64_t s_numCreated_;   // Timer对象的全局递增序列号
};

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_TIMER_