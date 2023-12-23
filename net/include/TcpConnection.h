#ifndef CSERVER_NET_INCLUDE_TCPCONNECTION_
#define CSERVER_NET_INCLUDE_TCPCONNECTION_

#include <memory>
#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "noncopyable.h"

namespace cServer {

int getSocketError(int sockfd);

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection> {
 public:
  // 构造函数，由TcpServer在接受新连接时调用
  TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr);
  // 析构函数，释放资源
  ~TcpConnection();

  // 获取事件循环对象指针
  EventLoop *getLoop() const {
    return loop_;
  }

  // 获取连接的名称
  const std::string &name() const {
    return name_;
  }

  // 获取本地地址
  const InetAddress &localAddress() {
    return localAddr_;
  }
  
  // 获取对端地址
  const InetAddress &peerAddress() {
    return peerAddr_;
  }

  // 判断连接是否已建立
  bool connected() const {
    return state_ == kConnected;
  }

  // void send(const void* message, size_t len);
  // 线程安全
  void send(const std::string& message);    // 发消息
  // 线程安全
  void shutdown();                          // 半关闭（关闭写）
  void setTcpNoDelay(bool on);              // 用于设置TCP_NODELAY选项，启用或禁用Nagle算法

  // 设置连接建立时的回调函数
  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }

  // 设置消息到达时的回调函数
  void setMessageCallback(const MessageCallback &cb) {
    messageCallback_ = cb;
  }

void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
  // 设置发送缓冲区清空时调用的回调函数
  writeCompleteCallback_ = cb;
}

  // 仅供内部使用。
  // 设置连接关闭时的回调函数
  void setCloseCallback(const CloseCallback &cb) {
    closeCallback_ = cb;
  }

  // 当TcpServer接受新连接时调用，用于完成连接的建立
  void connectEstablished();    // 应该仅被调用一次
  // 当TcpServer将TcpConnection从其映射中移除时调用，表示连接已销毁
  void connectDestroyed();      // 应该只被调用一次

 private:
  // 表示连接状态的枚举
  enum StateE {
    kConnecting,      // 初始状态
    kConnected,       // connectEstablished()后的状态
    kDisconnecting,   // shutdown()后的状态
    kDisconnected,    // handleClose()后的状态
  };

  // 设置连接状态
  void setState(StateE s) {
    state_ = s;
  }
  
  void handleRead(Timestamp receiveTime);         // 处理读事件
  void handleWrite();                             // 处理写事件
  void handleClose();                             // 处理连接关闭事件
  void handleError();                             // 处理连接错误事件
  void sendInLoop(const std::string& message);    // 在事件循环中发送消息。
  void shutdownInLoop();                          // 在事件循环中半关闭套间字（关闭写）。

  // 保存事件循环对象指针
  EventLoop *loop_;
  // 连接的名称
  std::string name_;
  // 连接的状态
  StateE state_;
  // 套接字对象的智能指针，管理连接的套接字资源
  std::unique_ptr<Socket> socket_;
  // Channel对象的智能指针，用于注册和处理事件
  std::unique_ptr<Channel> channel_;
  // 本地地址
  InetAddress localAddr_;
  // 对端地址
  InetAddress peerAddr_;
  // 连接建立和断开连接时的回调函数
  ConnectionCallback connectionCallback_;
  // 消息到达时的回调函数
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;     // 如果发送缓冲区清空就调用它
  // 连接关闭回调函数，这个回调是给TcpServer和TcpClient用的，用于通知它们移除所持有的TcpConnectionPtr
  CloseCallback closeCallback_;
  Buffer inputBuffer_;    // 定义读缓冲区
  Buffer outputBuffer_;   // 定义写缓冲区
};

// TcpConnection类的智能指针类型
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_TCPCONNECTION_