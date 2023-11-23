#ifndef CSERVER_TOOL_INCLUDE_MUTEX_
#define CSERVER_TOOL_INCLUDE_MUTEX_

#include <assert.h>
#include <pthread.h>

#include "noncopyable.h"
#include "CurrentThread.h"

namespace cServer {

class MutexLock : noncopyable {
 public:
  MutexLock();

  ~MutexLock();

  bool isLockedByThisThread();

  void assertLocked();

  void lock();

  void unlock();

  pthread_mutex_t* getPthreadMutex();

 private:
  pthread_mutex_t mutex_;
  pid_t holder_;
};

class MutexLockGuard : noncopyable {
 public:
  explicit MutexLockGuard(MutexLock& mutex) : mutex_(mutex) {
    mutex_.lock();
  }

  ~MutexLockGuard() {
    mutex_.unlock();
  }

 private:
  MutexLock& mutex_;
};

}  // namespace cServer

// Prevent misuse like:
// MutexLockGuard(mutex_);
// A tempory object doesn't hold the lock for long!
#define MutexLockGuard(x) error "Missing guard object name"
// 这个宏的作用是防止程序里出现如下错误：
/*
void doit() {
  muduo::MutexLock mutex;
  muduo::MutexLockGuard(mutex);  //
遗漏变量名，产生一个临时对象又马上销毁了，结果没锁住临界区
  // 正确写法是 MutexLockGuard lock(mutex)
}
*/

#endif  // CSERVER_TOOL_INCLUDE_MUTEX_
