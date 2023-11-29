#include <functional>
#include "AsyncLogging.h"
#include "LogFile.h"
#include "Timestamp.h"

namespace cServer {
AsyncLogging::AsyncLogging(const string &basename, off_t rollSize, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogging::threadFunc, this)),
      latch_(1),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_() {
  currentBuffer_->bzero();  // 清空
  nextBuffer_->bzero();     // 清空
  buffers_.reserve(16);
}

// 前端在生成一条日志消息的时候会调用AsyncLogging::append()。
void AsyncLogging::append(const char *logline, int len) {
  cServer::MutexLockGuard lock(mutex_);  // 使用互斥锁保护对共享数据的访问

  // 检查当前缓冲区是否有足够的空间容纳日志消息
  if (currentBuffer_->avail() > len) {
    // 如果有足够的空间，则直接将日志消息追加到当前缓冲区
    currentBuffer_->append(logline, len);
  } else {
    // 如果当前缓冲区空间不足，把它送入（移入）buffers_
    buffers_.push_back(std::move(currentBuffer_));

    // 检查是否有预备缓冲区，如果有，则将其作为当前缓冲区。即试图把预备好的另一块缓冲（nextBuffer_）移用（move）为当前缓冲
    if (nextBuffer_) {
      currentBuffer_ = std::move(nextBuffer_);
    } else {
      // 如果没有预备缓冲区，则创建一个新的缓冲区（这种情况很少发生）
      currentBuffer_.reset(new Buffer);  // Rarely happens
    }
    // 将日志消息追加到当前缓冲区
    currentBuffer_->append(logline, len);
    // 通知后台线程有新的日志消息需要写入文件
    cond_.notify();
  }
}

void AsyncLogging::threadFunc() {
  assert(running_ == true);  // 确保异步日志线程正在运行
  latch_.countDown();  // 减少启动倒计时，通知主线程线程已启动

  // 创建日志文件对象，准备写入日志文件
  LogFile output(basename_, rollSize_, false);
  // 首先准备好两块空闲的buffer，以备在临界区内交换
  BufferPtr newBuffer1(new Buffer);  // 创建第一个新缓冲区
  BufferPtr newBuffer2(new Buffer);  // 创建第二个新缓冲区
  newBuffer1->bzero();
  newBuffer2->bzero();
  BufferVector buffersToWrite;  // 用于存储待写入文件的缓冲区集合
  buffersToWrite.reserve(16);
  while (running_) {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      cServer::MutexLockGuard lock(mutex_);  // 临界区
      if (buffers_.empty())  // unusual usage!  异常情况：缓冲区为空
      {
        // 在临界区内，等待条件触发，条件有两个：其一是超时，其二是前端写满了一个或多个buffer。
        // 这里是非常规的condition
        // variable用法，它没有使用while循环，而且等待时间有上限。
        cond_.waitForSeconds(flushInterval_);  // 等待一段时间，等待有新的日志消息到达
      }
      buffers_.push_back(std::move(currentBuffer_));  // 将当前缓冲（currentBuffer_）移入buffers_
      currentBuffer_ = std::move(newBuffer1);  // 将空闲的newBuffer1移为当前缓冲
      buffersToWrite.swap(buffers_);  // 将buffers_与buffersToWrite交换，后面的代码可以在临界区之外安全地访问buffersToWrite
      if (!nextBuffer_) {
        // 用newBuffer2替换nextBuffer_，这样前端始终有一个预备buffer可供调配。nextBuffer_可以减少前端临界区分配内存的概率，缩短前端临界区长度。
        nextBuffer_ = std::move(newBuffer2);
      }
    }

    assert(!buffersToWrite.empty());

    if (buffersToWrite.size() > 25) {
      // 万一前端陷入死循环，拼命发送日志消息（缓冲区数量超过25个），
      // 超过后端的处理（输出）能力，直接丢掉多余的日志buffer，以腾出内存
      char buf[256];
      snprintf(
          buf, sizeof(buf), "Dropped log messages at %s, %zd larger buffers\n",
          Timestamp::now().toFormatString().c_str(), buffersToWrite.size() - 2);
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
    }

    for (const auto &buffer : buffersToWrite) {
      // 将待写入文件的缓冲区内容追加到日志文件
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffer->data(), buffer->length());
    }

    if (buffersToWrite.size() > 2) {
      // 截断多余的非零缓冲区，避免占用过多内存
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);
    }

    // 将buffersToWrite内的buffer重新填充newBuffer1和newBuffer2，这样下一次执行的时候还有两个空闲buffer可用于替换前端的当前缓冲和预备缓冲。
    if (!newBuffer1) {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2) {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();  // 清空容器，即删除容器内的std::unique_ptr<Buffer>，由于是unique_ptr，所以同时会释放指针指向的内存
    output.flush();  // 刷新日志文件，确保写入磁盘
  }
  output.flush();  // 在线程结束前，再次刷新日志文件，确保所有日志都被写入磁盘
}
}  // namespace cServer
