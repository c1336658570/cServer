#include <pthread.h>
#include <errno.h>
#include "Condition.h"
#include "./Mutex.h"
#include "noncopyable.h"

namespace cServer {

// 析构函数，销毁条件变量
Condition::~Condition() {
  pthread_cond_destroy(&pcond_);
}

// 等待条件变量，会阻塞当前线程直到被通知
void Condition::wait() {
  pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
}

// 等待一段时间，超时返回true，否则返回false
// 参数seconds表示等待的秒数
bool Condition::waitForSeconds(int seconds) {
  // 创建一个 timespec 结构体变量 abstime，用于表示时间。
  struct timespec abstime;
  // 获取当前的系统时间，CLOCK_REALTIME 表示获取的是系统实时时间。
  clock_gettime(CLOCK_REALTIME, &abstime);
  //  将 abstime 中的秒数成员加上 seconds，表示在当前时间的基础上再增加指定的等待秒数，得到超时时刻。
  abstime.tv_sec += seconds;
  // 使用pthread_cond_timedwait实现带超时的等待。这个函数会在等待时阻塞当前线程，直到条件变量 pcond_ 被通知或者超时。
  // &abstime: 超时时刻，如果在这个时刻之前条件变量被通知，线程就会被唤醒，否则在超时后返回。
  return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
}

// 唤醒一个等待的线程
void Condition::notify() {
  pthread_cond_signal(&pcond_);
}

// 唤醒所有等待的线程
void Condition::notifyAll() {
  pthread_cond_broadcast(&pcond_);
}

} // namespace cServer
