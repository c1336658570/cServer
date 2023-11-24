#include "Thread.h"
#include "Logging.h"

namespace cServer {

// 定义一个结构体，用于保存线程的数据
struct ThreadData {
  typedef cServer::Thread::ThreadFunc ThreadFunc;
  ThreadFunc func_;       // 要由线程执行的函数
  pid_t* tid_;            // 用于存储线程ID的指针
  CountDownLatch* latch_; // 用于同步的CountDownLatch指针

  ThreadData(ThreadFunc func, pid_t* tid, CountDownLatch* latch) :
  func_(std::move(func)), tid_(tid), latch_(latch) {}

  void runInThread() {
    *tid_ = cServer::CurrentThread::tid();    // 获取并存储线程ID
    tid_ = NULL;
    latch_->countDown();                      // 减少latch计数，表示线程已启动
    latch_ = NULL;
    func_();                                  // 执行线程函数
  }
};

// 由pthread_create调用的启动线程的函数
void* startThread(void* obj) {
  ThreadData* data = static_cast<ThreadData*>(obj);
  data->runInThread();      // 执行线程函数
  delete data;              // 释放为ThreadData分配的内存
  return NULL;
}

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func)
    : started_(false),
      joined_(false),
      pthreadId_(0),
      tid_(0),
      func_(std::move(func)),
      latch_(1) {
}

Thread::~Thread() {
  // 如果线程已启动但回收，则分离线程
  if (started_ && !joined_) {
    pthread_detach(pthreadId_);
  }
}

// 启动线程
void Thread::start() {
  assert(!started_);
  started_ = true;      // 标记线程已启动
  // 创建ThreadData结构并启动线程
  ThreadData* data = new ThreadData(func_, &tid_, &latch_);
  if (pthread_create(&pthreadId_, NULL, &startThread, data)) {
    started_ = false;   // 在失败的情况下标记线程为未启动
    delete data;        // 释放为ThreadData分配的内存
    LOG_SYSFATAL << "Failed in pthread_create";
  } else {
    latch_.wait();        // 等待线程信号启动
    assert(tid_ > 0);
  }
}

// 回收线程
int Thread::join() {
  assert(started_);   // 确保线程已启动
  assert(!joined_);   // 确保线程尚未回收
  joined_ = true;
  return pthread_join(pthreadId_, NULL);    // 回收线程
}

}  // namespace cServer
