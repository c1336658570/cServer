#include <functional>
#include "EventLoopThread.h"
#include "EventLoop.h"

namespace cServer {
EventLoopThread::EventLoopThread()
    : loop_(NULL),      // 初始化事件循环指针为空
      exiting_(false),  // 初始化线程退出标志为false
      thread_(std::bind(&EventLoopThread::threadFunc, this)),  // 创建线程对象，绑定线程函数
      mutex_(),                  // 初始化互斥锁
      cond_(mutex_)              // 初始化条件变量，与互斥锁关联
{}

// EventLoopThread 对象的析构函数，用于清理资源。
EventLoopThread::~EventLoopThread() {
  exiting_ = true;  // 设置线程退出标志为true
  loop_->quit();  // 通过事件循环指针调用quit()函数退出事件循环
  thread_.join();  // 等待事件循环线程结束
}

// 启动事件循环线程，并等待事件循环对象的创建完成，返回事件循环的指针。
EventLoop *EventLoopThread::startLoop() {
  assert(!thread_.started());  // 断言事件循环线程未启动
  thread_.start();             // 启动事件循环线程

  {
    // 使用互斥锁保护对事件循环指针的访问
    MutexLockGuard lock(mutex_);
    // 等待事件循环对象的创建完成
    while (loop_ == NULL) {
      cond_.wait();
    }
  }

  return loop_;  // 返回事件循环的指针
}

void EventLoopThread::threadFunc() {
  // 创建事件循环对象
  EventLoop loop;

  {
    // 使用互斥锁保护对事件循环指针的访问
    MutexLockGuard lock(mutex_);
    loop_ = &loop;   // 设置事件循环指针
    cond_.notify();  // 通知主线程事件循环对象的创建完成
  }

  loop.loop();  // 运行事件循环
  // assert(exiting_);
}

}  // namespace cServer
