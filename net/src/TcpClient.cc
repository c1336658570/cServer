#include "TcpClient.h"
#include "Connector.h"
#include "EventLoop.h"
#include "Logging.h"
#include "InetAddress.h"
#include "Callbacks.h"

#include <functional>

#include <cstdio>  // snprintf

namespace cServer {

void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn) {
  // 在事件循环的线程中将连接销毁的操作添加到事件队列中
  loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr &connector) {}


TcpClient::TcpClient(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr)),
      retry_(false),
      connect_(true),
      nextConnId_(1) {
  // 设置新连接建立时的回调函数
  connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
  LOG_INFO << "TcpClient::TcpClient[" << this << "] - connector " << connector_.get();
}

TcpClient::~TcpClient() {
  LOG_INFO << "TcpClient::~TcpClient[" << this << "] - connector " << connector_.get();
  TcpConnectionPtr conn;
  {
    MutexLockGuard lock(mutex_);
    conn = connection_;
    // 销毁TcpClient对象时，先获取当前连接，并在事件循环的线程中设置连接关闭的回调函数
  }
  if (conn) {
    CloseCallback cb = std::bind(&cServer::removeConnection, loop_, std::placeholders::_1);
    loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
  } else {
    // 如果当前没有连接，则停止连接器的操作，并在稍后的时间在事件循环的线程中移除连接器
    connector_->stop();
    loop_->runAfter(1, std::bind(&cServer::removeConnector, connector_));
  }
}

void TcpClient::connect() {
  // 连接到服务器，启动连接器
  LOG_INFO << "TcpClient::connect[" << this << "] - connecting to " << connector_->serverAddress().toHostPort();
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect() {
  // 断开连接
  connect_ = false;

  {
    MutexLockGuard lock(mutex_);
    if (connection_) {
      // 关闭连接
      connection_->shutdown();
    }
  }
}

void TcpClient::stop() {
  connect_ = false;
  // 停止连接
  connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
  // 在事件循环的线程中处理新连接建立事件
  loop_->assertInLoopThread();
  InetAddress peerAddr(getPeerAddr(sockfd));
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toHostPort().c_str(), nextConnId_);
  ++nextConnId_;
  string connName = buf;

  InetAddress localAddr(getLocalAddr(sockfd));
  // 创建一个新的TcpConnection对象，并设置相应的回调函数
  TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));

  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, std::placeholders::_1));
  {
    MutexLockGuard lock(mutex_);
    connection_ = conn;
  }
  // 建立连接完成，调用连接建立完成的回调函数
  conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn) {
  // 在事件循环的线程中处理连接的移除操作
  loop_->assertInLoopThread();
  assert(loop_ == conn->getLoop());

  {
    MutexLockGuard lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  // 将连接销毁的操作添加到事件队列中
  loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  if (retry_ && connect_) {
    // 如果需要重试并且允许连接，则重新启动连接器进行重连
    LOG_INFO << "TcpClient::connect[" << this << "] - Reconnecting to " << connector_->serverAddress().toHostPort();
    connector_->restart();
  }
}

}  // namespace cServer
