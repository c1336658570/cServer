#include <functional>
#include <cstdio>
#include "TcpServer.h"
#include "Logging.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"

namespace cServer {

// TcpServer构造函数，用于初始化TcpServer对象
TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr) :
loop_(loop),                                  // 设置TcpServer所属的EventLoop
name_(listenAddr.toHostPort()),               // 使用监听地址生成服务器名称
threadPool_(new EventLoopThreadPool(loop)),
acceptor_(new Acceptor(loop, listenAddr)),    // 创建Acceptor对象，用于监听新连接
started_(false),                              // 服务器初始状态为未启动
nextConnId_(1) {                              // 下一个连接的ID从1开始
  // 设置Acceptor的新连接回调函数，当有新连接时调用TcpServer的newConnection函数
  acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

// TcpServer析构函数
TcpServer::~TcpServer() {
}

void TcpServer::setThreadNum(int numThreads) {
  assert(0 <= numThreads);
  threadPool_->setThreadNum(numThreads);
}

// 启动服务器
void TcpServer::start() {
  if (!started_) {    // 如果服务器尚未启动
    started_ = true;  // 设置服务器状态为已启动
    threadPool_->start();
  }

  if (!acceptor_->listenning()) {  // 如果Acceptor尚未监听
    // 在事件循环线程中执行Acceptor的listen函数，开始监听新连接
    loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
  }
}

// 处理新连接的函数
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
  loop_->assertInLoopThread();                        // 确保在事件循环线程中调用
  char buf[32];
  snprintf(buf, sizeof(buf), "#%d", nextConnId_);     // 生成连接名称
  ++nextConnId_;
  std::string connName = name_ + buf;                 // 使用服务器名称和连接编号生成完整连接名称

  // 打印日志，记录新连接的信息
  LOG_INFO << "TcpServer::newConnection [" << name_<< "] - new connection [" 
           << connName << "] from " << peerAddr.toHostPort();
  InetAddress localAddr(getLocalAddr(sockfd));   // 获取本地地址
  EventLoop *ioLoop = threadPool_->getNextLoop();     // 获得下一个EventLoop
  // 创建TcpConnection对象
  TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
  connections_[connName] = conn;                      // 将连接对象添加到连接映射中
  conn->setConnectionCallback(connectionCallback_);   // 设置连接回调函数
  conn->setMessageCallback(messageCallback_);         // 设置消息回调函数
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  // 设置关闭时回调函数
  conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
  ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));   // 让ioLoop调用connectEstablished
}

// 从TcpServer的连接映射中移除指定的TcpConnection对象
 void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
  // 在TcpConnection自己所属的io EventLoop中会调用handleClose，在handleClose中会调用closeCallback_
  // 而closeCallback_就是removeConnection，我们需要把移除connection移动到loop_(即baseLoop_)中
  loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

// 从TcpServer的连接映射中移除指定的TcpConnection对象
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn){
  // 断言确保在TcpServer所属的EventLoop所属的线程中调用该函数
  loop_->assertInLoopThread();
  // 记录日志，标明正在移除连接
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name();
  // 从连接映射中移除指定的TcpConnection对象
  size_t n = connections_.erase(conn->name());
  assert(n == 1); (void)n;    // 断言确保只移除了一个 TcpConnection
  EventLoop *ioLoop = conn->getLoop();
  // 在所属的io EventLoop中执行连接销毁操作，通过queueInLoop确保在下一次事件循环中执行
  ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));  // 使用std::bind让TcpConnect声明其长到调用connectDestroyed()的时刻
}

}  // namespace cServer
