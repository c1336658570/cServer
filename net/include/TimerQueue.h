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
  void cancel(TimerId timerId);   // 取消定时器

 private:
  typedef std::pair<Timestamp, Timer *> Entry;
  /*
   * TimerQueue::TimerList是一个std::set，并且lower_bound使用默认的operator<来比较元素。
   * 在默认情况下，对于std::pair类型，它会比较第一个元素，即Timestamp类型，来确定元素的顺序。
   */
  typedef std::set<Entry> TimerList;
  // 表示活跃的定时器，包含 Timer 指针和定时器的序列号
  typedef std::pair<Timer *, int64_t> ActiveTimer;
  // 表示活跃的定时器集合，按照地址排序
  typedef std::set<ActiveTimer> ActiveTimerSet;

  void addTimerInLoop(Timer *timer);    // 在EventLoop中添加定时器
  void cancelInLoop(TimerId timerId);   // 在EventLoop中取消定时器

  // 当timerfd的时间到期时调用
  void handleRead();

  // 移除所有已过期的定时器
  std::vector<Entry> getExpired(Timestamp now);
  // 重置定时器列表，将重复定时器重新插入
  void reset(const std::vector<Entry> &expired, Timestamp now);

  // 插入一个定时器到定时器列表中
  bool insert(Timer *timer);

  EventLoop *loop_;           // 关联的EventLoop
  const int timerfd_;         // 使用的timerfd描述符
  Channel timerfdChannel_;    // 使用了一个Channel来观察timerfd_上的readable事件。
  TimerList timers_;          // 按到期时间排序的定时器列表

  // for cancel()
  bool callingExpiredTimers_; // 原子的，表示当前定时器的回调是否正在被调用(handleRead)
  // 当前有效的Timer的指针，和timers保存的是相同的数据，所以timers.size()==activeTimers_.size()，activeTimers_内的元素是按地址排序的
  ActiveTimerSet activeTimers_;
  // cancelingTimers_ 是一个 ActiveTimerSet 集合，用于存储正在取消中的定时器
  ActiveTimerSet cancelingTimers_;
};

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_TIMERQUEUE_