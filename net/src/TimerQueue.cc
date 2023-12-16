#include <ctime>
#include <sys/timerfd.h>
#include <unistd.h>
#include <cassert>
#include <functional>
#include "TimerQueue.h"
#include "Logging.h"
#include "Timer.h"
#include "EventLoop.h"
#include "TimerId.h"

namespace cServer {

// 创建一个timerfd文件描述符
int createTimerfd() {
  // 使用::timerfd_create函数创建一个定时器文件描述符，时钟类型为CLOCK_MONOTONIC，选项为TFD_NONBLOCK和TFD_CLOEXEC
  // CLOCK_MONOTONIC表示使用单调时钟，不受系统时间调整的影响。
  // TFD_NONBLOCK将文件描述符设置为非阻塞模式，使得对timerfd的读写操作变为非阻塞。
  // TFD_CLOEXEC在exec函数调用时关闭文件描述符，防止子进程继承。
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  // 检查timerfd_create是否成功，若失败则输出错误信息并终止程序
  if (timerfd < 0) {
    LOG_SYSFATAL << "Failed in timerfd_create";
  }
  return timerfd;  // 返回创建的timerfd文件描述符
}

// 计算距离给定时间戳（when）还有多少时间
struct timespec howMuchTimeFromNow(Timestamp when) {
  // 计算当前时间戳距离目标时间戳的微秒数
  int64_t microseconds = when.microsecondsSinceEpoch() - Timestamp::now().microsecondsSinceEpoch();
  if (microseconds < 100) {
    // 如果微秒数小于100，则将其设置为100微秒，以避免计时器设置为过于短的时间
    microseconds = 100;
  }
  // 定义struct timespec结构体，用于表示时间
  struct timespec ts;
  // 计算秒数部分并赋值给ts结构体的tv_sec
  ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
  // 计算纳秒数部分并赋值给ts结构体的tv_nsec
  ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
  return ts;    // 返回表示距离目标时间戳还有多少时间的timespec结构体
}

// 从timerfd文件描述符读取数据，处理定时器到期事件
void readTimerfd(int timerfd, Timestamp now) {
  // 用于存储从timerfd中读取的数据
  uint64_t howmany;
  // 从timerfd中读取数据，将结果存储在howmany变量中
  // read从fd中读取1个无符号8byte整型（uint64_t，主机字节序，存放当buf中），表示超时的次数。
  // 如果没有超时，read将会阻塞到下一次定时器超时，或者失败（errno设置为EAGAIN，fd设置为非阻塞）。
  // 另外，如果提供的buffer大小 < 8byte，read将返回EINVAL。read成功时，返回值应为8。
  ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
  // 输出日志，记录定时器到期事件的发生时间和读取的数据量
  LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
  // 检查读取的数据量是否为8字节，若不是则输出错误日志
  if (n != sizeof(howmany)) {
    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
  }
}

// 重置timerfd文件描述符，用于更新定时器的到期时间
void resetTimerfd(int timerfd, Timestamp expiration) {
  // 唤醒EventLoop，通过timerfd_settime()函数设置新的定时器到期时间
  struct itimerspec newValue;       // 用于存储新的定时器配置
  struct itimerspec oldValue;       // 用于存储旧的定时器配置

  // 将newValue和oldValue结构体清零
  bzero(&newValue, sizeof(newValue));
  bzero(&oldValue, sizeof(oldValue));

  newValue.it_value = howMuchTimeFromNow(expiration); // 设置新的定时器到期时间

  // 使用timerfd_settime()函数设置新的定时器到期时间，并将旧的定时器到期时间存储在oldValue中
  int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
  // 检查timerfd_settime是否成功，若失败则输出系统错误信息
  if (ret) {
    LOG_SYSERR << "timerfd_settime()";
  }
}

// TimerQueue构造函数，初始化TimerQueue对象
TimerQueue::TimerQueue(EventLoop *loop) :
      loop_(loop),                          // 设置当前TimerQueue所属的EventLoop
      timerfd_(createTimerfd()),            // 创建timerfd文件描述符
      timerfdChannel_(loop, timerfd_),      // 创建与timerfd相关联的Channel
      timers_() {
  // 设置timerfdChannel的读回调函数为TimerQueue::handleRead()
  timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
  // 始终监听timerfd文件描述符的可读事件，通过timerfd_settime()来停止定时器
  timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
  ::close(timerfd_);    // 关闭计时器文件描述符
  // 不用移除channel，因为我们在 EventLoop::dtor() 中
  for (TimerList::iterator it = timers_.begin(); it != timers_.end(); ++it) {
    delete it->second;
  }
}

// 向定时器队列中添加定时器的函数，创建定时器对象并异步加入事件循环中。
// - cb: 定时器到期时要执行的回调函数
// - when: 定时器的到期时间戳
// - interval: 定时器的重复间隔时间
TimerId TimerQueue::addTimer(const TimerCallback &cb,
                             Timestamp when,
                             double interval)
{
  // 创建一个新的定时器对象
  Timer *timer = new Timer(cb, when, interval);
  // 异步地将定时器对象加入事件循环中
  loop_->runInLoop(
      std::bind(&TimerQueue::addTimerInLoop, this, timer));
  // 返回定时器的唯一标识 TimerId
  return TimerId(timer);
}

// 在事件循环中添加定时器的函数，确保在事件循环所在的IO线程中执行。
// - timer: 要添加的定时器对象指针
void TimerQueue::addTimerInLoop(Timer *timer)
{
  // 断言当前线程是事件循环所在的IO线程
  loop_->assertInLoopThread();
  // 将定时器插入到定时器队列中，并记录是否导致最早到期时间的改变
  bool earliestChanged = insert(timer);

  // 如果最早到期时间发生了改变，重置定时器文件描述符的超时时间
  if (earliestChanged)
  {
    resetTimerfd(timerfd_, timer->expiration());
  }
}

// 处理timerfd文件描述符可读事件的回调函数
void TimerQueue::handleRead() {
  loop_->assertInLoopThread();
  Timestamp now(Timestamp::now());
  readTimerfd(timerfd_, now);         // 读取timerfd文件描述符的数据

  // 获取已过期的定时器列表
  std::vector<Entry> expired = getExpired(now);

  // safe to callback outside critical section
  // 在安全的情况下执行已过期定时器的回调函数
  for (std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it) {
    it->second->run();
  }

  // 如果是重复定时器，重新设置定时器列表中剩余定时器的到期时间
  reset(expired, now);
}

// 函数会从timers_中移除已到期的Timer，并通过vector返回它们。
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
  std::vector<Entry> expired;  // 创建一个用于存储已过期定时器的vector
  // 创建一个临时的定时器条目（sentry），其到期时间为当前时间，用于辅助
  // lower_bound 查找
  Entry sentry = std::make_pair(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
  /*
   * 使用 lower_bound
   * 查找第一个未到期的定时器迭代器，确保找到的定时器时间大于当前时间
   * TimerQueue::TimerList是一个std::set，并且lower_bound使用默认的operator<来比较元素。
   * 在默认情况下，对于std::pair类型，它会比较第一个元素，即Timestamp类型，来确定元素的顺序。
   */
  // 让set::lower_bound()返回的是第一个未到期的Timer的迭代器，因此的断言中是<而非≤。
  TimerList::iterator it = timers_.lower_bound(sentry);
  assert(it == timers_.end() || now < it->first);
  // 将已过期的定时器复制到expired中
  std::copy(timers_.begin(), it, back_inserter(expired));
  // 从 timers_ 中移除已过期的定时器
  timers_.erase(timers_.begin(), it);

  return expired;  // 返回包含已过期定时器的vector
}

// 如果是重复定时器，重新设置定时器列表中剩余定时器的到期时间
void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now) {
  Timestamp nextExpire;

  // 遍历已过期的定时器列表
  for (std::vector<Entry>::const_iterator it = expired.begin(); it != expired.end(); ++it) {
    if (it->second->repeat()) {
      // 如果是重复触发的定时器，重新启动并插入到TimerList中
      it->second->restart(now);
      insert(it->second);
    } else {
      // 不是重复触发的定时器，释放内存
      delete it->second;
    }
  }

   // 如果TimerList不为空，获取最早到期的定时器的到期时间
  if (!timers_.empty()) {
    nextExpire = timers_.begin()->second->expiration();
  }

  // 如果存在下一个到期时间，则重新设置timerfd文件描述符的到期时间
  if (nextExpire.isValid()) {
    resetTimerfd(timerfd_, nextExpire);
  }
}

// 向TimerList中插入定时器，并返回是否是最早到期的标志
bool TimerQueue::insert(Timer *timer) {
  bool earliestChanged = false;
  Timestamp when = timer->expiration();
  TimerList::iterator it = timers_.begin();
  // 如果TimerList为空或者新的定时器的到期时间更早，则更新earliestChanged为true
  if (it == timers_.end() || when < it->first) {
    earliestChanged = true;
  }
  // 将新的定时器插入TimerList中
  std::pair<TimerList::iterator, bool> result = timers_.insert(std::make_pair(when, timer));
  assert(result.second);      // 检查插入是否成功
  return earliestChanged;
}

} // namespace cServer