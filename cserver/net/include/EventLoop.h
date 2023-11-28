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

namespace cServer {

class Channel;
class Poller;
class TimerQueue;

// EventLoop是不可拷贝的，每个线程只能有一个EventLoop对象
// 创建了EventLoop对象的线程是IO线程，其主要功能是运行事件循环EventLoop:: loop()。
// EventLoop对象的生命期通常和其所属的线程一样长，它不必是heap对象。
class EventLoop : noncopyable {
 public:
  EventLoop();
  ~EventLoop();

  void loop();

  void quit();

  // 获取poll返回的时间戳，通常表示数据到达的时间。
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  // 这几个EventLoop成员函数应该允许跨线程使用，比方说我想在某个IO线程中执行超时回调。

  // 在指定的时间'time'执行回调函数'cb'。
  TimerId runAt(const Timestamp& time, const TimerCallback& cb);
  // 在延迟'delay'秒后执行回调函数'cb'。
  TimerId runAfter(double delay, const TimerCallback& cb);
  // 每隔'interval'秒执行一次回调函数'cb'。
  TimerId runEvery(double interval, const TimerCallback& cb);

  // void cancel(TimerId timerId);

  void updateChannel(Channel* channel);
  // void removeChannel(Channel* channel);

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

  typedef std::vector<Channel *> ChannelList;

  // 事件循环(loop)是否正在运行的标志（原子操作）
  bool looping_;    // atomic
  bool quit_;       // atomic
  const pid_t threadId_;
  Timestamp pollReturnTime_;
  std::unique_ptr<Poller> poller_;  // EventLoop的poller对象
  std::unique_ptr<TimerQueue> timerQueue_;    // 定时事件队列
  ChannelList activeChannels_;      // 活动的channel
};

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_EVENTLOOP_