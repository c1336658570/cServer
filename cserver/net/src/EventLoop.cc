#include <cassert>
#include <poll.h>
#include "EventLoop.h"
#include "Logging.h"
#include "Poller.h"
#include "Channel.h"

namespace cServer {

__thread EventLoop *t_loopInThisThread = 0;
const int kPollTimeMs = 10000;

EventLoop::EventLoop() : looping_(false), quit_(false), 
                         threadId_(CurrentThread::tid()), poller_(new Poller(this)) {
  LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
  // 检查当前线程是否已经创建了其他EventLoop对象
  if (t_loopInThisThread) {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread << " exits in this thread" << threadId_;
  } else {
    t_loopInThisThread = this;    // 记住本对象所属的线程（threadId_）。
  }
}

EventLoop::~EventLoop() {
  assert(!looping_);
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
    activeChannels_.clear();
    poller_->poll(kPollTimeMs, &activeChannels_);   // 调用Poller::poll()获得当前活动事件的Channel列表
    for (ChannelList::iterator it = activeChannels_.begin(); it != activeChannels_.end(); ++it) {
      // 依次调用每个Channel的handleEvent()函数。
      (*it)->handleEvent();
    }
  }
  LOG_TRACE << "EventLoop " << this << " stop looping";   // 输出日志，表示事件循环结束
  looping_ = false;         // 将事件循环状态设置为非运行状态
}

void EventLoop::quit() {
  quit_ = true;   // 将quit_设为true终止事件循环
  // wakeup();
}

void EventLoop::updateChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);  // 调用Poller的updateChannel()来更新Channel的状态（监听读/写/或不监听）
}

void EventLoop::abortNotInLoopThread() {
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " << CurrentThread::tid();
}

}  // namespace cServer