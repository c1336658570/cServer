#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Logging.h"

namespace cServer {

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
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));   // 设置读回调函数为handleRead
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

  connectionCallback_(shared_from_this());  // 调用连接建立时的回调函数
}

// 处理读事件，当有数据可读时被调用
void TcpConnection::handleRead()
{
  char buf[65536];
  ssize_t n = ::read(channel_->fd(), buf, sizeof buf);    // 从套接字读取数据
  messageCallback_(shared_from_this(), buf, n);           // 调用消息到达时的回调函数
}

}
