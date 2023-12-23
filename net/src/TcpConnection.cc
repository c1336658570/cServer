#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Logging.h"

namespace cServer {

int getSocketError(int sockfd) {
  int optval;
  socklen_t optlen = sizeof(optval);

  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  }
  else {
    return optval;
  }
}

// TcpConnection构造函数，用于初始化TcpConnection对象
TcpConnection::TcpConnection(EventLoop *loop, const std::string &nameArg, int sockfd,
                             const InetAddress &localAddr, const InetAddress &peerAddr) :
loop_(loop),                          // 初始化所属的Eventloop
name_(nameArg),                       // TcpConnection名
state_(kConnecting),                  // 连接状态，连接中和已连接状态
socket_(new Socket(sockfd)),          // 创建Socket对象，管理连接的套接字资源
channel_(new Channel(loop, sockfd)),  // 创建Channel对象，用于注册和处理事件
localAddr_(localAddr),                // 初始化本地地址
peerAddr_(peerAddr) {                 // 初始化对端地址
  LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this << " fd=" << sockfd;
  // 设置 Channel 的读、写、关闭、错误事件回调函数
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));   // 设置读回调函数为handleRead
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
}

// TcpConnection析构函数，释放资源
// TcpConnection拥有TCP socket，它的析构函数会close(fd)（在Socket的析构函数中发生）。
TcpConnection::~TcpConnection() {
  LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this << " fd=" << channel_->fd();
}

// 在TCP连接上发送消息。如果连接处于已连接状态（kConnected），
// 则根据当前线程是否为事件循环线程来选择直接发送消息或者通过事件循环线程发送消息。
void TcpConnection::send(const std::string &message) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message);
    } else {
      loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, message));
    }
  }
}

// 在事件循环线程中实际发送消息。该函数被设计为在事件循环线程中执行，负责实际的消息发送逻辑。
// 首先尝试直接写入数据到套接字，如果不成功则将剩余数据加入输出缓冲区，并启用写事件。
void TcpConnection::sendInLoop(const std::string &message) {
  /*
   * 先尝试直接发送数据，如果一次发送完毕就不会启用WriteCallback；如果只发送了部分数据，
   * 则把剩余的数据放入outputBuffer_，并开始关注writable事件，以后在handlerWrite()中发送剩余的数据。
   * 如果当前outputBuffer_已经有待发送的数据，那么就不能先尝试发送了，因为这会造成数据乱序。
   */
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  // 如果输出队列中没有数据，尝试直接写入
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    nwrote = ::write(channel_->fd(), message.data(), message.size());
    if (nwrote >= 0) {
      if (static_cast<size_t>(nwrote) < message.size()) {
        LOG_TRACE << "I am going to write more data";
      } else if (writeCompleteCallback_) {
        // 一次性发送完了所有数据，调用writeCompleteCallback_，writeCompleteCallback_是发送缓冲区清空的回调函数
        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
      }
    } else {
      // 如果当前outputBuffer_已经有待发送的数据，那么就不能先尝试发送了，因为这会造成数据乱序。
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG_SYSERR << "TcpConnection::sendInLoop";
      }
    }
  }

  // 如果未写完所有数据，将剩余数据加入输出缓冲区，并启用写事件
  assert(nwrote >= 0);
  if (static_cast<size_t>(nwrote) < message.size()) {
    outputBuffer_.append(message.data() + nwrote, message.size() - nwrote);
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

// 半关闭。关闭写。如果连接处于已连接状态（kConnected），则将连接状态置为断开中（kDisconnecting），
// 并通过事件循环线程执行关闭逻辑。
void TcpConnection::shutdown() {
  // FIXME: use compare and swap
  if (state_ == kConnected) {
    setState(kDisconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

// 在事件循环线程中执行关闭逻辑。该函数被设计为在事件循环线程中执行，负责实际的关闭逻辑，包括关闭写端口。
void TcpConnection::shutdownInLoop() {
  loop_->assertInLoopThread();
  if (!channel_->isWriting()) {
    // 如果当前没有在写数据，则关闭写端口
    socket_->shutdownWrite();
  }
}

// 用于设置TCP_NODELAY选项，启用或禁用Nagle算法
void TcpConnection::setTcpNoDelay(bool on) {
  socket_->setTcpNoDelay(on);
}

// 连接建立时调用，用于完成连接的建立
void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();        // 确保在IO线程中调用
  assert(state_ == kConnecting);      // 确保当前状态为连接中
  setState(kConnected);               // 设置连接状态为已连接
  channel_->enableReading();          // 启动读监听

  connectionCallback_(shared_from_this());  // 调用连接建立和断开连接时的回调函数
}

// 处理连接销毁事件，connectDestroyed()是TcpConnection析构前最后调用的一个成员函数，它通知用户连接已断开。
void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();
  assert(state_ == kConnected || state_ == kDisconnecting);   // 检查是否是正常连接状态或半关闭状态
  setState(kDisconnected);
  // 此处的disableAll和handleClose的disableALL是重复的，因为有时候connectDestroyed不经由handleclose调用，而是直接调用connectDestroyed()
  channel_->disableAll();     // 禁用 Channel 的所有事件关注
  connectionCallback_(shared_from_this());      // 调用连接建立和断开连接时的回调函数

  loop_->removeChannel(channel_.get());         // 从 EventLoop 中移除 Channel
}

// 处理读事件，当有数据可读时被调用
void TcpConnection::handleRead(Timestamp receiveTime) {
  int savedErrno = 0;   // 保存错误号
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);   // 从套接字读取数据到输入缓冲区
  if (n > 0) {
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);     // 调用消息到达回调函数
  } else if (n == 0) {
    handleClose();        // 处理连接关闭事件
  } else {
    errno = savedErrno;
    LOG_SYSERR << "TcpConnection::handleRead";
    handleError();        // 处理连接错误事件
  }
}

// 处理写事件。该函数负责处理套接字的写事件。在事件循环线程中调用，用于实现数据的异步写入。
// 如果当前连接正在写数据（channel_->isWriting() 为真），则尝试将输出缓冲区的数据写入套接字。
// 该函数假设调用时连接已经确保处于事件循环线程中。
void TcpConnection::handleWrite() {
  loop_->assertInLoopThread();

  // 检查连接是否正在监听写事件
  if (channel_->isWriting()) {
    ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
    if (n > 0) {
      // 已成功写入部分或全部数据，更新输出缓冲区
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0) {
        // 如果输出缓冲区已经为空，则禁用写事件，如果处在断开连接状态下则执行关闭逻辑
        channel_->disableWriting();
        if (writeCompleteCallback_) {
          // 发送缓冲区已经为空了，调用发送缓冲区清空的回调函数
          loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == kDisconnecting) {
          shutdownInLoop();
        }
      } else {
        LOG_TRACE << "I am going to write more data";
      }
    } else {
      // 写入失败，记录错误信息
      LOG_SYSERR << "TcpConnection::handleWrite";
    }
  } else {
    // 连接没有在写数据，记录日志
    LOG_TRACE << "Connection is down, no more writing";
  }
}

// 处理连接关闭事件，主要是调用closeCallback_，这个回调绑定到TcpServer::removeConnection()
void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  LOG_TRACE << "TcpConnection::handleClose state = " << state_;
  assert(state_ == kConnected || state_ == kDisconnecting);
  // 不关闭文件描述符，在析构函数中关闭，方便查找内存泄漏
  channel_->disableAll();     // 禁用 Channel 的所有事件关注
  closeCallback_(shared_from_this());     // 调用连接关闭回调函数
}

// 处理连接错误事件，只是在日志中输出错误消息，这不影响连接的正常关闭
void TcpConnection::handleError() {
  int err = getSocketError(channel_->fd());
  LOG_ERROR << "TcpConnection::handleError [" << name_
            << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

}
