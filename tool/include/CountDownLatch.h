#ifndef CSERVER_TOOL_INCLUDE_COUNTDOWNLATCH_
#define CSERVER_TOOL_INCLUDE_COUNTDOWNLATCH_

#include "Mutex.h"
#include "Condition.h"

#include "noncopyable.h"

namespace cServer {

class CountDownLatch : noncopyable {
 public:

  // 如果一个class要包含MutexLock和Condition，请注意它们的声明顺序和初始化顺序，
  // mutex_应先于condition_构造，并作为后者的构造参数
  explicit CountDownLatch(int count) :  // 倒数几次
      mutex_(),
      condition_(mutex_),               // 初始化顺序要与成员声明保持一致
      count_(count) { }

  void wait() {   // 等待计数值变为1
    MutexLockGuard lock(mutex_);
    while (count_ > 0) {
      condition_.wait();
    }
  }

  void countDown() { // 计数减1
    MutexLockGuard lock(mutex_);
    --count_;
    if (count_ == 0) {
      condition_.notifyAll();   // 唤醒所有阻塞在该条件变量上的线程
    }
  }

  int getCount() const {
    MutexLockGuard lock(mutex_);
    return count_;
  }

 private:
  mutable MutexLock mutex_;     // 顺序很重要，先mutex后condition
  Condition condition_;
  int count_;
};

} // namespace cServer
#endif  // CSERVER_TOOL_INCLUDE_COUNTDOWNLATCH_
