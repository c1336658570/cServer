#include "Acceptor.h"
#include "InetAddress.h"
#include "EventLoop.h"
#include "Logging.h"
#include "Socket.h"

namespace cServer {

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr) :
loop_(loop), acceptSocket_(createNonblocking()), acceptChannel_(loop, acceptSocket_.fd()), listenning_(false) {
  acceptSocket_.setReuseAddr(true);           // 设置套接字选项，允许地址重用
  acceptSocket_.bindAddress(listenAddr);      // 绑定地址和端口
  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));   // 设置可读事件回调函数
}

// 开始监听连接请求。
void Acceptor::listen() {
  loop_->assertInLoopThread();      // 确保在事件循环线程中调用
  listenning_ = true;               // 标记正在监听连接
  acceptSocket_.listen();           // 开始监听连接请求
  acceptChannel_.enableReading();   // 开启可读事件监听，并将其注册到poll中
}

// 处理可读事件，表示有新的连接请求到达。
void Acceptor::handleRead() {
  loop_->assertInLoopThread();  // 确保在事件循环线程中调用
  InetAddress peerAddr(0);      // 用于保存对端地址
  int connfd = acceptSocket_.accept(&peerAddr);  // 接受连接，直到没有更多连接请求
  if (connfd >= 0) {
    // 如果设置了新连接回调函数，则调用回调函数处理新连接
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd, peerAddr);
    } else {
      // 否则关闭连接
      ::close(connfd);
    }
  }
}

}  // namespace cServer
