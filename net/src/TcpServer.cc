#include <functional>
#include <cstdio>
#include "TcpServer.h"
#include "Logging.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"

namespace cServer {

// TcpServer构造函数，用于初始化TcpServer对象
TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr) :
loop_(loop),                                  // 设置TcpServer所属的EventLoop
name_(listenAddr.toHostPort()),               // 使用监听地址生成服务器名称
acceptor_(new Acceptor(loop, listenAddr)),    // 创建Acceptor对象，用于监听新连接
started_(false),                              // 服务器初始状态为未启动
nextConnId_(1) {                              // 下一个连接的ID从1开始
  // 设置Acceptor的新连接回调函数，当有新连接时调用TcpServer的newConnection函数
  acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

// TcpServer析构函数
TcpServer::~TcpServer() {
}

// 启动服务器
void TcpServer::start() {
  if (!started_) {    // 如果服务器尚未启动
    started_ = true;  // 设置服务器状态为已启动
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

  // 创建TcpConnection对象
  TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));
  connections_[connName] = conn;                      // 将连接对象添加到连接映射中
  conn->setConnectionCallback(connectionCallback_);   // 设置连接回调函数
  conn->setMessageCallback(messageCallback_);         // 设置消息回调函数
  conn->connectEstablished();                         // 调用连接建立函数，完成连接的建立
}

}  // namespace cServer
