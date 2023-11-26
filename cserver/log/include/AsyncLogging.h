#ifndef CSERVER_LOG_INCLUDE_ASYNCLOGGING_
#define CSERVER_LOG_INCLUDE_ASYNCLOGGING_

#include <atomic>
#include <memory>

#include "LogStream.h"
#include "Thread.h"
#include "noncopyable.h"

namespace cServer {

// 异步日志类，用于在后台线程中异步写入日志
class AsyncLogging : noncopyable {
 public:
  // 构造函数，参数为日志文件的基本名称、滚动大小和刷新间隔（默认为3秒）
  AsyncLogging(const string& basename, off_t rollSize, int flushInterval = 3);
  // 析构函数，如果后台线程正在运行，则停止它
  ~AsyncLogging() {
    if (running_) {
      stop();
    }
  }

  // 添加日志消息到待写入的缓冲区
  void append(const char* logline, int len);

  // 启动异步日志线程
  void start() {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  // 停止异步日志线程
  void stop() {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:
  // 后台线程函数，用于定期将日志写入文件
  void threadFunc();

  // 定义一个缓冲区类型，大小为4MB，可存储1000条日志消息
  typedef cServer::FixedBuffer<kLargeBuffer> Buffer;  // Buffer是FixedBuffer的具体实现，大小4MB，可以存1000条日志消息
  // 定义缓冲区的智能指针类型
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  // 别名定义，方便使用
  typedef BufferVector::value_type BufferPtr;

  const int flushInterval_;        // 刷新间隔，单位秒
  std::atomic<bool> running_;      // 表示异步日志线程是否在运行
  const string basename_;          // 日志文件的基本名称
  const off_t rollSize_;           // 日志文件滚动大小
  cServer::Thread thread_;         // 后台线程对象
  cServer::CountDownLatch latch_;  // 用于等待后台线程启动完成
  cServer::MutexLock mutex_;       // 互斥锁，用于保护以下成员变量

  cServer::Condition cond_;  // 条件变量，用于线程间的通信
  BufferPtr currentBuffer_;  // 当前缓冲
  BufferPtr nextBuffer_;     // 预备缓冲
  BufferVector buffers_;     // 存放的是供后端写入的buffer
};

}  // namespace cServer

#endif // CSERVER_LOG_INCLUDE_ASYNCLOGGING_