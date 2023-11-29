#ifndef CSERVER_NET_INCLUDE_TIMERQUEUE_
#define CSERVER_NET_INCLUDE_TIMERQUEUE_

#include <vector>
#include <set>
#include "noncopyable.h"
#include "Callbacks.h"
#include "Timestamp.h"
#include "Channel.h"

namespace cServer {

// 前向声明，避免包含头文件
class EventLoop;
class Timer;
class TimerId;

class TimerQueue : noncopyable {
 public:
  // 构造函数：初始化TimerQueue，传入关联的EventLoop。
  TimerQueue(EventLoop *loop);
  // 析构函数：销毁 TimerQueue。
  ~TimerQueue();

  // 将回调函数安排在给定时间运行，如果interval > 0.0，则重复执行。
  // 必须是线程安全的。通常从其他线程调用。addTimer()是供EventLoop使用的。
  // 向timers_中插入一个定时器，cb是定时器到期时执行的回调函数。when表示到期事件，interval用于表示定时器是否是循环定时器
  TimerId addTimer(const TimerCallback &cb, Timestamp when, double interval);
  // void cancel(TimerId timerId);

 private:
  typedef std::pair<Timestamp, Timer *> Entry;
  /*
   * TimerQueue::TimerList是一个std::set，并且lower_bound使用默认的operator<来比较元素。
   * 在默认情况下，对于std::pair类型，它会比较第一个元素，即Timestamp类型，来确定元素的顺序。
   */
  typedef std::set<Entry> TimerList;

  void addTimerInLoop(Timer* timer);

  // 当timerfd的时间到期时调用
  void handleRead();

  // 移除所有已过期的定时器
  std::vector<Entry> getExpired(Timestamp now);
  // 重置定时器列表，将未过期的定时器重新插入
  void reset(const std::vector<Entry> &expired, Timestamp now);

  // 插入一个定时器到定时器列表中
  bool insert(Timer *timer);

  EventLoop *loop_;           // 关联的EventLoop
  const int timerfd_;         // 使用的timerfd描述符
  Channel timerfdChannel_;    // 使用了一个Channel来观察timerfd_上的readable事件。
  TimerList timers_;          // 按到期时间排序的定时器列表
};

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_TIMERQUEUE_