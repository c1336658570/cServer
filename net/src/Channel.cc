#include <poll.h>
#include "Channel.h"
#include "Logging.h"
#include "EventLoop.h"

namespace cServer {

/*
 * POLLIN：表示普通数据可读。当文件描述符上有普通数据可以读取时触发。
 * POLLPRI：表示优先数据可读。通常用于表示紧急或带外数据可读，例如带外数据到达时触发。
 * POLLRDHUP：表示对端关闭连接，或者写半部关闭。当检测到对端关闭连接时触发。
 * POLLERR：表示发生错误。当有错误发生时触发，可以通过 revents 字段进一步检查错误类型。
 * POLLNVAL：表示请求的事件类型或文件描述符不是有效的。当传递给 poll() 的文件描述符无效时触发。
 * POLLOUT：表示普通数据可写。当文件描述符上可以写入数据而不会阻塞时触发。
 */

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

// loop此channel所属的EventLoop，fdArg，此channel管理的文件描述符
Channel::Channel(EventLoop *loop, int fdArg) : loop_(loop), fd_(fdArg), 
events_(0), revents_(0), index_(-1), eventHandling_(false) {
}

// Channel 类的析构函数，确保在处理事件时不会被析构
Channel::~Channel() {
  assert(!eventHandling_);
}

// 调用EventLoop::updateChannel()，后者会转而调用Poller::updateChannel()。
// 由于Channel.h没有包含EventLoop.h，因此Channel::update()必须定义在Channel.cc中。
void Channel::update() {
  loop_->updateChannel(this);
}

// Channel::handleEvent()是Channel的核心，它由EventLoop::loop()调用，根据revents_的值分别调用不同的用户回调。
void Channel::handleEvent() {
  // 标识当前正在处理事件
  eventHandling_ = true;

  // 检查POLLNVAL事件（无效的请求）
  if (revents_ & POLLNVAL) {
    LOG_WARN << "Channel::handle_event() POLLNVAL";
  }

  // 处理 POLLHUP 事件，同时确保没有发生 POLLIN 事件
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    LOG_WARN << "Channel::handle_event() POLLHUP";
    if (closeCallback_) {
      closeCallback_();
    }
  }

  // 检查POLLERR和POLLNVAL事件（错误事件）
  if (revents_ & (POLLERR | POLLNVAL)) {
    if (errorCallback_) {
      errorCallback_();
    }
  }

  // 检查POLLIN、POLLPRI、POLLRDHUP事件（可读事件）  
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (readCallback_) {
      readCallback_();
    }
  }

  // 检查POLLOUT事件（可写事件）  
  if (revents_ & POLLOUT) {
    if (writeCallback_) {
      writeCallback_();
    }
  }

  // 标识事件处理完成
  eventHandling_ = false;
}

} // namespace cServer
