#ifndef CSERVER_NET_INCLUDE_EVENTLOOPTHREAD_
#define CSERVER_NET_INCLUDE_EVENTLOOPTHREAD_

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"
#include "noncopyable.h"

namespace cServer {

class EventLoop;

class EventLoopThread : noncopyable {
 public:
  EventLoopThread();
  ~EventLoopThread();

  // 启动事件循环线程，返回事件循环的指针。
  EventLoop* startLoop();

 private:
  // 事件循环线程的线程函数，创建事件循环对象，运行事件循环。
  void threadFunc();

  EventLoop* loop_;   // 指向事件循环对象的指针。
  bool exiting_;      // 表示线程是否正在退出。
  Thread thread_;     // 线程对象，用于管理事件循环线程。
  MutexLock mutex_;   // 互斥锁，用于保护对loop_和exiting_的访问。
  Condition cond_;    // 条件变量，等待子线程创建完毕
};

}  // namespace cServer

#endif  // CSERVER_NET_INCLUDE_EVENTLOOPTHREAD_
