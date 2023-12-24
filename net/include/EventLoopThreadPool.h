#ifndef CSERVER_NET_INCLUDE_EVENTLOOPTHREADPOLL_
#define CSERVER_NET_INCLUDE_EVENTLOOPTHREADPOLL_

#include <functional>
#include <memory>
#include "noncopyable.h"
#include <vector>

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"

namespace cServer {

// 前置声明，避免循环引用
class EventLoop;
class EventLoopThread;

// EventLoopThreadPool类，继承自noncopyable防止拷贝
class EventLoopThreadPool : noncopyable {
 public:
 // 构造函数，接受一个基础EventLoop参数
  EventLoopThreadPool(EventLoop *baseLoop);
  // 析构函数
  ~EventLoopThreadPool();
  // 设置EventLoopThread线程数量的函数
  void setThreadNum(int numThreads) {
    numThreads_ = numThreads;
  }

  // 启动线程池的函数
  void start();
  // 获取下一个EventLoop的函数
  // TcpServer每次新建一个TcpConnection就会调用getNextLoop()来取得EventLoop，
  // 如果是单线程服务，每次返回的都是baseLoop_，即TcpServer自己用的那个loop。
  EventLoop* getNextLoop();

 private:
 // 使用typedef定义类型别名
  typedef std::unique_ptr<EventLoopThread> EventLoopThreadPtr;
  typedef std::vector<EventLoopThreadPtr> ptr_vector;
  EventLoop *baseLoop_;             // 基础EventLoop指针
  bool started_;                    // 线程池是否已启动的标志
  int numThreads_;                  // 线程数量
  int next_;                        // 下一个要分配任务的线程索引，始终在循环线程中
  ptr_vector threads_;              // 存储线程对象的容器
  std::vector<EventLoop*> loops_;   // 存储EventLoop指针的容器
};

}  // namespace cServer

#endif  // CSERVER_NET_INCLUDE_EVENTLOOPTHREADPOLL_
