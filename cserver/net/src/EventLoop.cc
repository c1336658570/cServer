#include <cassert>
#include <poll.h>
#include "EventLoop.h"
#include "Logging.h"

namespace cServer {

__thread EventLoop* t_loopInThisThread = 0;

EventLoop::EventLoop() : looping_(false), threadId_(CurrentThread::tid()) {
  LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
  // 检查当前线程是否已经创建了其他EventLoop对象
  if (t_loopInThisThread) {
    LOG_FATAL << "Another EventLoop" << t_loopInThisThread << " exits in this thread" << threadId_;
  } else {
    t_loopInThisThread = this;    // 记住本对象所属的线程（threadId_）。
  }
}

EventLoop::~EventLoop() {
  assert(!looping_);
  t_loopInThisThread = NULL;
}

// 每个线程至多有一个EventLoop对象，让EventLoop的static成员函数getEventLoopOfCurrentThread()返回这个对象。
EventLoop* EventLoop::getEventLoopOfCurrentThread() {
  return t_loopInThisThread;
}

// 事件循环的主函数，什么事都不做，等5秒就退出
void EventLoop::loop() {
  assert(!looping_);        // 确保事件循环处于未运行状态
  assertInLoopThread();     // 确保当前调用线程为EventLoop对象所属的线程
  looping_ = true;          // 将事件循环状态设置为运行中

  ::poll(NULL, 0, 5*1000);  // 等5s

  LOG_TRACE << "EventLoop " << this << " stop looping";   // 输出日志，表示事件循环结束
  looping_ = false;         // 将事件循环状态设置为非运行状态
}

void EventLoop::abortNotInLoopThread() {
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " << CurrentThread::tid();
}

}  // namespace cServer