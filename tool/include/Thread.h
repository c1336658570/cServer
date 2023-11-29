#ifndef CSERVER_TOOL_INCLUDE_THREAD_
#define CSERVER_TOOL_INCLUDE_THREAD_

#include <atomic>
#include <functional>
#include <string>

#include "CountDownLatch.h"
#include "noncopyable.h"

namespace cServer {

class Thread : noncopyable {
 public:
  typedef std::function<void()> ThreadFunc;
  explicit Thread(ThreadFunc);
  ~Thread();

  void start();
  int join();

  bool started() const {
    return started_;
  }

  pthread_t pthreadId() const {
    return pthreadId_;
  }

  pid_t tid() const {
    return tid_;
  }

  static int numCreated() {
    return numCreated_;
  }

 private:
  bool started_;            // 标志线程是否已经启动
  bool joined_;             // 标志线程是否已经回收
  pthread_t pthreadId_;     // 线程的pthread ID
  pid_t tid_;               // 线程的tid（线程ID）
  ThreadFunc func_;         // 要在线程中执行的函数
  CountDownLatch latch_;    // 用于同步的计数器

  static std::atomic_int numCreated_;   // 统计已创建的Thread对象数量
};

}  // namespace cServer

#endif  // CSERVER_TOOL_INCLUDE_THREAD_