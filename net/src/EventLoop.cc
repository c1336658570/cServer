#include <cassert>
#include <poll.h>
#include <sys/eventfd.h>
#include <functional>
#include <signal.h>
#include "EventLoop.h"
#include "Logging.h"
#include "Poller.h"
#include "Channel.h"
#include "TimerQueue.h"

namespace cServer {

__thread EventLoop *t_loopInThisThread = 0;
const int kPollTimeMs = 10000;

// 创建并配置一个eventfd文件描述符，用于在多线程间传递事件通知。
// 返回值：int，表示创建的eventfd文件描述符
static int createEventfd()
{
  // 使用系统调用eventfd创建一个eventfd文件描述符。
  // 参数1：初始计数器的值，通常初始化为0。
  // 参数2：标志，EFD_NONBLOCK表示非阻塞模式，EFD_CLOEXEC表示在执行exec()时关闭文件描述符。
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  // 检查是否成功创建eventfd文件描述符。
  if (evtfd < 0)
  {
    // 如果创建失败，记录错误信息到日志。
    LOG_SYSERR << "Failed in eventfd";
    // 中止程序运行。
    abort();
  }
  // 返回创建的eventfd文件描述符。
  return evtfd;
}

// 屏蔽SIGPIPE信号，避免断开连接，服务器发送数据，导致服务器挂
class IgnoreSigPipe {
 public:
  IgnoreSigPipe() {
  ::signal(SIGPIPE, SIG_IGN);
  }
};

IgnoreSigPipe initObj;

EventLoop::EventLoop()
  : looping_(false),                    // 初始化loop_为未开始
    quit_(false),                       // 初始化退出状态为未退出
    callingPendingFunctors_(false),     // 初始化回调函数处理状态为未处理
    threadId_(CurrentThread::tid()),    // 获取当前线程的线程ID
    poller_(new Poller(this)),          // 创建一个用于轮询事件的Poller对象
    timerQueue_(new TimerQueue(this)),  // 创建一个定时器队列TimerQueue对象
    wakeupFd_(createEventfd()),         // 创建一个用于唤醒事件循环的eventfd文件描述符
    wakeupChannel_(new Channel(this, wakeupFd_)) { // 创建一个Channel对象用于处理eventfd的可读事件
  // 在日志中记录EventLoop对象的创建信息，包括对象地址和所属线程ID。
  LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
  // 检查当前线程是否已经创建了其他EventLoop对象
  if (t_loopInThisThread) {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread << " exits in this thread" << threadId_;
  } else {
    t_loopInThisThread = this;    // 记住本对象所属的线程（threadId_）。
  }
  // 设置wakeupChannel的读回调函数为EventLoop::handleRead()。
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
  // 启用wakeupChannel的读事件，表示始终监听eventfd的可读事件。
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
  assert(!looping_);              // 确保事件循环未在运行
  ::close(wakeupFd_);             // 关闭用于唤醒事件循环的文件描述符
  // 将线程局部变量t_loopInThisThread置为空指针，表示当前线程不再拥有EventLoop对象
  t_loopInThisThread = NULL;
}

// 每个线程至多有一个EventLoop对象，让EventLoop的static成员函数getEventLoopOfCurrentThread()返回这个对象。
EventLoop *EventLoop::getEventLoopOfCurrentThread() {
  return t_loopInThisThread;
}

// 事件循环的主函数
void EventLoop::loop() {
  assert(!looping_);        // 确保事件循环处于未运行状态
  assertInLoopThread();     // 确保当前调用线程为EventLoop对象所属的线程
  looping_ = true;          // 将事件循环状态设置为运行中
  quit_ = false;

  while (!quit_) {
    // 清空活跃channel列表和轮询返回时间
    activeChannels_.clear();
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);   // 调用Poller::poll()获得当前活动事件的Channel列表
    for (ChannelList::iterator it = activeChannels_.begin(); it != activeChannels_.end(); ++it) {
      // 依次调用每个Channel的handleEvent()函数。
      (*it)->handleEvent(pollReturnTime_);
    }
    doPendingFunctors();    // 执行等待中的回调函数
  }
  LOG_TRACE << "EventLoop " << this << " stop looping";   // 输出日志，表示事件循环结束
  looping_ = false;         // 将事件循环状态设置为非运行状态
}

void EventLoop::quit() {
  quit_ = true;   // 将quit_设为true终止事件循环
  // 如果调用quit()的线程不是IO线程，则唤醒IO线程
  if (!isInLoopThread())
  {
    wakeup();
  }
}

// 在事件循环中运行回调函数，如果在当前IO线程调用，回调将同步进行，否则将回调加入队列，并唤醒IO线程执行。
// cb表示要运行的回调函数
void EventLoop::runInLoop(const Functor &cb)
{
  // 如果在当前IO线程调用，回调将同步进行
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    // 如果在其他线程调用，将回调加入队列，并唤醒IO线程执行
    queueInLoop(cb);
  }
}

// 将回调函数加入队列，并在必要时唤醒IO线程。
void EventLoop::queueInLoop(const Functor &cb)
{
  {
  // 使用互斥锁保护对等待中回调函数队列的操作
  MutexLockGuard lock(mutex_);
  pendingFunctors_.push_back(cb);
  }

  // 如果调用queueInLoop()的线程不是IO线程，或者正在调用等待中回调函数，唤醒IO线程
  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}


TimerId EventLoop::runAt(const Timestamp &time, const TimerCallback &cb) {
  // 在指定时间'time'执行回调函数'cb'，定时器重复间隔为0.0表示单次触发
  return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback &cb) {
  // 计算延迟'delay'后的时间点
  Timestamp time(addTime(Timestamp::now(), delay));
  // 在计算得到的时间点执行回调函数'cb'
  return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback &cb) {
  // 计算第一次执行回调函数'cb'的时间点
  Timestamp time(addTime(Timestamp::now(), interval));
  // 设置定时器，每隔'interval'秒执行一次回调函数'cb'
  return timerQueue_->addTimer(cb, time, interval);
}

// 取消一个定时器
void EventLoop::cancel(TimerId timerId) {
  return timerQueue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);  // 调用Poller的updateChannel()来更新Channel的状态（监听读/写/或不监听）
}

// 从 EventLoop 中移除指定的 Channel 对象
void EventLoop::removeChannel(Channel* channel) {
  assert(channel->ownerLoop() == this);   // 断言确保要移除的 Channel 属于当前 EventLoop
  assertInLoopThread();                   // 断言确保在当前 EventLoop 线程中调用该函数
  poller_->removeChannel(channel);        // 调用Poller的removeChannel函数，将Channel从事件管理中移除
}

void EventLoop::abortNotInLoopThread() {
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " << CurrentThread::tid();
}

// 唤醒事件循环的函数，向唤醒文件描述符写入一个64位整数（无实际意义），用于唤醒事件循环中的阻塞。
void EventLoop::wakeup()
{
  uint64_t one = 1;
  // 使用write()向唤醒文件描述符写入一个64位整数
  ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
  // 检查写入的字节数是否为8，如果不是，记录错误信息到日志
  if (n != sizeof(one))
  {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

// 处理唤醒事件的函数，从唤醒文件描述符读取一个64位整数，用于清除文件描述符上的可读事件。
void EventLoop::handleRead()
{
  uint64_t one = 1;
  // 使用read()从唤醒文件描述符读取一个64位整数
  ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
  // 检查读取的字节数是否为8，如果不是，记录错误信息到日志
  if (n != sizeof(one))
  {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

// 执行等待中回调函数的函数，将等待中的回调函数队列中的回调函数取出并逐个执行。
void EventLoop::doPendingFunctors()
{
  // 临时存储等待中的回调函数队列
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  // 把回调列表swap()到局部变量functors中，这样一方面减小了临界区的长度（意味着不会阻塞其他线程调用queueInLoop()），
  // 另一方面也避免了死锁（因为Functor可能再调用queueInLoop()）。
  {
  // 使用互斥锁保护对等待中回调函数队列的操作
  MutexLockGuard lock(mutex_);
  functors.swap(pendingFunctors_);
  }

  // 遍历回调函数队列，逐个执行回调函数
  for (size_t i = 0; i < functors.size(); ++i)
  {
    functors[i]();
  }
  // 将回调函数处理状态置为false
  callingPendingFunctors_ = false;
}

}  // namespace cServer