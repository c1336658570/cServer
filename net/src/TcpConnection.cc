#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Logging.h"

namespace cServer {

int getSocketError(int sockfd) {
  int optval;
  socklen_t optlen = sizeof optval;

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
TcpConnection::~TcpConnection()
{
  LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this << " fd=" << channel_->fd();
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
  assert(state_ == kConnected);
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

// 处理写事件
void TcpConnection::handleWrite() {
}

// 处理连接关闭事件，主要是调用closeCallback_，这个回调绑定到TcpServer::removeConnection()
void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  LOG_TRACE << "TcpConnection::handleClose state = " << state_;
  assert(state_ == kConnected);
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
