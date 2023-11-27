#ifndef CSERVER_TOOL_INCLUDE_CONDITION_
#define CSERVER_TOOL_INCLUDE_CONDITION_

#include "./Mutex.h"

#include "noncopyable.h"
#include <pthread.h>
#include <errno.h>

namespace cServer {

class Condition : noncopyable {
 public:
  // 构造函数，需要传入一个互斥锁对象
  explicit Condition(MutexLock &mutex) : mutex_(mutex) {
    // 初始化条件变量
    pthread_cond_init(&pcond_, NULL);
  }

  // 析构函数，销毁条件变量
  ~Condition();

  // 等待条件变量，会阻塞当前线程直到被通知
  void wait();

  // 等待一段时间，超时返回true，否则返回false
  // 参数seconds表示等待的秒数
  bool waitForSeconds(int seconds);

  // 唤醒一个等待的线程
  void notify();

  // 唤醒所有等待的线程
  void notifyAll();

 private:
  MutexLock &mutex_;        // 引用互斥锁对象，用于与条件变量一起使用
  pthread_cond_t pcond_;    // 条件变量对象
};

} // namespace cServer
#endif  // CSERVER_TOOL_INCLUDE_CONDITION_
