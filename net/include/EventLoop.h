#ifndef CSERVER_NET_INCLUDE_EVENTLOOP_
#define CSERVER_NET_INCLUDE_EVENTLOOP_

#include <unistd.h>
#include <memory>
#include <vector>
#include "noncopyable.h"
#include "CurrentThread.h"
#include "Callbacks.h"
#include "Timestamp.h"
#include "TimerId.h"
#include "Mutex.h"

namespace cServer {

class Channel;
class Poller;
class TimerQueue;

// EventLoop是不可拷贝的，每个线程只能有一个EventLoop对象
// 创建了EventLoop对象的线程是IO线程，其主要功能是运行事件循环EventLoop:: loop()。
// EventLoop对象的生命期通常和其所属的线程一样长，它不必是heap对象。
class EventLoop : noncopyable {
 public:
  typedef std::function<void()> Functor;

  EventLoop();
  ~EventLoop();

  void loop();

  void quit();    // 退出事件循环

  // 获取poll返回的时间戳，通常表示数据到达的时间。
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  // 如果用户在当前IO线程调用这个函数，回调会同步进行；
  // 如果用户在其他线程调用runInLoop()，cb会被加入队列，IO线程会被唤醒来调用这个Functor。
  void runInLoop(const Functor &cb);
  // 在循环线程中排队回调。
  void queueInLoop(const Functor &cb);

  // 这几个EventLoop成员函数应该允许跨线程使用，比方说我想在某个IO线程中执行超时回调。

  // 在指定的时间'time'执行回调函数'cb'。在其他线程调用是线程安全的
  TimerId runAt(const Timestamp &time, const TimerCallback &cb);
  // 在延迟'delay'秒后执行回调函数'cb'。在其他线程调用是线程安全的
  TimerId runAfter(double delay, const TimerCallback &cb);
  // 每隔'interval'秒执行一次回调函数'cb'。在其他线程调用是线程安全的
  TimerId runEvery(double interval, const TimerCallback &cb);

  // void cancel(TimerId timerId);

  void wakeup();    // 唤醒

  void updateChannel(Channel *channel);
  // void removeChannel(Channel *channel);

  // 检查当前调用线程是否为 EventLoop 对象所属的线程  
  void assertInLoopThread() {
    if (!isInLoopThread()) {
      abortNotInLoopThread();
    }
  }

  // 检查当前对象所属的线程是否为调用线程
  bool isInLoopThread() const {
    return threadId_ == CurrentThread::tid();
  }

  // 获取当前线程的 EventLoop 对象指针  
  static EventLoop *getEventLoopOfCurrentThread();

 private:
  // 中止程序并输出错误消息，用于在非法线程中调用 assertInLoopThread() 时使用
  void abortNotInLoopThread();
  void handleRead();          // 处理读事件，用于唤醒
  void doPendingFunctors();   // 执行等待中的回调函数

  typedef std::vector<Channel *> ChannelList;

  // 事件循环(loop)是否正在运行的标志（原子操作）
  bool looping_;    // 是否在事件循环中
  bool quit_;       // 是否退出事件循环
  bool callingPendingFunctors_;     // 是否正在调用等待中的回调函数
  const pid_t threadId_;            // IO线程的线程ID
  Timestamp pollReturnTime_;        // poll返回的时间戳
  std::unique_ptr<Poller> poller_;  // Poller对象，用于事件的轮询
  std::unique_ptr<TimerQueue> timerQueue_;    // 定时事件队列
  int wakeupFd_;                              // 用于唤醒的文件描述符
  // 与TimerQueue不同，该类不会将Channel暴露给客户端。
  std::unique_ptr<Channel> wakeupChannel_;  // 用于处理wakeupFd_上的readable事件，将事件分发至handleRead()函数。
  ChannelList activeChannels_;              // 活动的channel
  MutexLock mutex_;                         // 互斥锁，保护等待中的回调函数队列
  std::vector<Functor> pendingFunctors_;    // mutex_ 等待中的回调函数队列
};

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_EVENTLOOP_