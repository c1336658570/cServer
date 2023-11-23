#include <assert.h>
#include <pthread.h>

#include "noncopyable.h"
#include "CurrentThread.h"
#include "Mutex.h"

namespace cServer {

MutexLock::MutexLock() : holder_(0) {
  pthread_mutex_init(&mutex_, NULL);
}

MutexLock::~MutexLock() {
  assert(holder_ == 0);
  pthread_mutex_destroy(&mutex_);
}

bool MutexLock::isLockedByThisThread() {
  return holder_ == CurrentThread::tid();
}

void MutexLock::assertLocked() {
  assert(isLockedByThisThread());
}

void MutexLock::lock() {      // 仅供MutexLockGuard调用，严禁用户代码调用
  pthread_mutex_lock(&mutex_);  // 这两行代码顺序不能反
  holder_ = CurrentThread::tid();
}

void MutexLock::unlock() {   // 仅供MutexLockGuard调用，严禁用户代码调用
  holder_ = 0;  // 这两行代码顺序不能反
  pthread_mutex_unlock(&mutex_);
}

pthread_mutex_t* MutexLock::getPthreadMutex() { /* non-const */  // 仅供MutexLockGuard调用，严禁用户代码调用
  return &mutex_;
}

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
