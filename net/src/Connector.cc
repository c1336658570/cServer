#include "Connector.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logging.h"
#include "Socket.h"
#include <functional>
#include <errno.h>

namespace cServer {

// Connector 类的静态常量，表示最大重试延迟时间
const int Connector::kMaxRetryDelayMs;

// Connector 类的构造函数，接受事件循环和服务器地址作为参数
Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs) {
  LOG_DEBUG << "ctor[" << this << "]";
}

// Connector 类的析构函数
Connector::~Connector() {
  LOG_DEBUG << "dtor[" << this << "]";
  loop_->cancel(timerId_);    // 取消定时器
  assert(!channel_);          // 确保channel_已经被释放
}

// 启动连接过程，可以在任意线程调用
void Connector::start() {
  connect_ = true;
  loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

// 在事件循环中启动连接过程
void Connector::startInLoop() {
  loop_->assertInLoopThread();        // 确保在事件循环线程中调用
  assert(state_ == kDisconnected);
  if (connect_) {
    connect();
  } else {
    LOG_DEBUG << "do not connect";
  }
}

// 发起连接
void Connector::connect() {
  int sockfd = createNonblocking();   // 创建非阻塞套接字
  // 必须创建临时变量addr，直接调用serverAddr_.getSockAddrInet()会导致无效参数，因为connect内部会修改第二个参数
  struct sockaddr_in addr = serverAddr_.getSockAddrInet();
  int ret = ::connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      // 以上错误表示正在连接,加入loop进行监听,直至连接成功
      connecting(sockfd);   // 连接中，调用connecting
      break;

    case EAGAIN:            // 错误代码与accept(2)不同，EAGAIN是真的错误
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      // 以上连接表示失败,需要关闭socketfd,重新创建并连接
      retry(sockfd);        // 重试连接
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
      if (::close(sockfd) < 0) {
        LOG_SYSERR << "close";
      }
      break;

    default:
      LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
      if (::close(sockfd) < 0) {
        LOG_SYSERR << "close";
      }
      break;
  }
}

// 重新启动连接过程
void Connector::restart() {
  loop_->assertInLoopThread();
  setState(kDisconnected);
  retryDelayMs_ = kInitRetryDelayMs;
  connect_ = true;
  startInLoop();
}

// 停止连接过程
void Connector::stop() {
  connect_ = false;
  loop_->cancel(timerId_);    // 取消定时器
}

// 处理连接中的回调
void Connector::connecting(int sockfd) {
  setState(kConnecting);
  assert(!channel_);
  channel_.reset(new Channel(loop_, sockfd));     // 创建channel并关联事件循环和套接字
  channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));   // 设置写事件回调
  channel_->setErrorCallback(std::bind(&Connector::handleError, this));   // 设置错误事件回调

  channel_->enableWriting();    // 启用写事件
}

// 移除并重置关联的channel_
int Connector::removeAndResetChannel() {
  channel_->disableAll();       // 禁用所有事件
  loop_->removeChannel(channel_.get());   // 从事件循环中移除channel_
  int sockfd = channel_->fd();            // 获取套接字文件描述符
  // 不能在这里重置 channel_，因为我们在 Channel::handleEvent 内部
  loop_->queueInLoop(std::bind(&Connector::resetChannel, this));    // 在事件循环中重置 channel_
  return sockfd;
}

// 重置关联的channel
void Connector::resetChannel() {
  channel_.reset();
}

// 处理写入事件的回调
void Connector::handleWrite() {
  LOG_TRACE << "Connector::handleWrite " << state_;

  if (state_ == kConnecting) {
    int sockfd = removeAndResetChannel();   // 移除并重置channel，获取套接字文件描述符
    int err = getSocketError(sockfd);       // 获取套接字错误
    if (err) {
      LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err << " " << strerror_tl(err);
      retry(sockfd);    // 发生错误，重试连接
    } else if (isSelfConnect(sockfd)) {
      LOG_WARN << "Connector::handleWrite - Self connect";
      retry(sockfd);    // 自连接，重试连接
    } else {
      setState(kConnected);   // 连接成功
      if (connect_) {
        newConnectionCallback_(sockfd);   // 调用新连接回调函数
      } else {
        if (::close(sockfd) < 0) {
          LOG_SYSERR << "close";
        }
      }
    }
  } else {
    assert(state_ == kDisconnected);
  }
}

// 处理错误事件的回调
void Connector::handleError() {
  LOG_ERROR << "Connector::handleError";
  assert(state_ == kConnecting);

  int sockfd = removeAndResetChannel();   // 移除并重置channel，获取套接字文件描述符
  int err = getSocketError(sockfd);       // 获取套接字错误
  LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
  retry(sockfd);    // 发生错误，重试连接
}

// 重试连接
void Connector::retry(int sockfd) {
  if (::close(sockfd) < 0) {
    LOG_SYSERR << "close";
  }
  setState(kDisconnected);
  if (connect_) {
    LOG_INFO << "Connector::retry - Retry connecting to "
             << serverAddr_.toHostPort() << " in " << retryDelayMs_
             << " milliseconds. ";
    timerId_ = loop_->runAfter(retryDelayMs_ / 1000.0, std::bind(&Connector::startInLoop, this));
    retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
  } else {
    LOG_DEBUG << "do not connect";
  }
}

}  // namespace cServer
