#ifndef CSERVER_NET_INCLUDE_TCPCONNECTION_
#define CSERVER_NET_INCLUDE_TCPCONNECTION_

#include <memory>
#include "Callbacks.h"
#include "InetAddress.h"
#include "noncopyable.h"

namespace cServer {

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

  // 设置连接建立时的回调函数
  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }

  // 设置消息到达时的回调函数
  void setMessageCallback(const MessageCallback &cb) {
    messageCallback_ = cb;
  }

  // 当TcpServer接受新连接时调用，用于完成连接的建立
  void connectEstablished();   // 应该仅被调用一次

 private:
  // 表示连接状态的枚举
  enum StateE {
    kConnecting,
    kConnected,
  };

  // 设置连接状态
  void setState(StateE s) {
    state_ = s;
  }
  
  // 处理读事件，当有数据可读时被调用
  void handleRead();

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
  // 连接建立时的回调函数
  ConnectionCallback connectionCallback_;
  // 消息到达时的回调函数
  MessageCallback messageCallback_;
};

// TcpConnection类的智能指针类型
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_TCPCONNECTION_