#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include <functional>

namespace cServer {
  // EventLoopThreadPool类的构造函数，初始化基础EventLoop指针、启动标志、线程数量和下一个线程索引
  EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop)
      : baseLoop_(baseLoop), started_(false), numThreads_(0), next_(0) {}

  // EventLoopThreadPool类的析构函数，注意不删除loop，因为它是栈变量
  EventLoopThreadPool::~EventLoopThreadPool() {
  }

  // EventLoopThreadPool类的启动函数，创建指定数量的线程并启动它们的事件循环
  void EventLoopThreadPool::start() {
    // 断言线程池未启动，确保在基础EventLoop的线程中调用
    assert(!started_);
    baseLoop_->assertInLoopThread();

    // 设置启动标志
    started_ = true;

    // 循环创建指定数量的线程，将它们添加到容器中，并获取它们的事件循环
    for (int i = 0; i < numThreads_; ++i) {
      EventLoopThread *t = new EventLoopThread;
      threads_.push_back(EventLoopThreadPtr(t));
      loops_.push_back(t->startLoop());
    }
  }

  // EventLoopThreadPool类的获取下一个EventLoop函数，实现简单的轮询分配
  // TcpServer每次新建一个TcpConnection就会调用getNextLoop()来取得EventLoop，
  // 如果是单线程服务，每次返回的都是baseLoop_，即TcpServer自己用的那个loop。
  EventLoop* EventLoopThreadPool::getNextLoop() {
    // 断言在基础EventLoop的线程中调用
    baseLoop_->assertInLoopThread();
    // 初始情况下，使用基础EventLoop
    EventLoop* loop = baseLoop_;

    // 如果存在其他事件循环，则使用轮询方式分配
    if (!loops_.empty()) {
      // 轮询
      loop = loops_[next_];
      ++next_;
      // 如果索引超过容器大小，重置为零
      if (static_cast<size_t>(next_) >= loops_.size()) {
        next_ = 0;
      }
    }
    // 返回选择的EventLoop指针
    return loop;
  }
}
