#ifndef CSERVER_NET_INCLUDE_TCPSERVER_
#define CSERVER_NET_INCLUDE_TCPSERVER_

#include <memory>
#include <map>
#include "Callbacks.h"
#include "TcpConnection.h"
#include "noncopyable.h"

namespace cServer {

class Acceptor;
class EventLoop;

// 管理accept(2)获得的TcpConnection。TcpServer是供用户直接使用的，生命期由用户控制。
// TcpServer类，用于管理TCP服务器
class TcpServer : noncopyable {
 public:
  // 构造函数，传入事件循环对象和监听地址
  TcpServer(EventLoop *loop, const InetAddress &listenAddr);
  // 析构函数，用于释放资源
  ~TcpServer();  // 强制定义在类外，为了处理unique_ptr成员

  // 启动服务器，如果尚未监听，可安全地多次调用，线程安全
  void start();

  // 设置连接回调函数，非线程安全
  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }

  // 设置消息回调函数，非线程安全
  void setMessageCallback(const MessageCallback& cb) {
    messageCallback_ = cb;
  }

  // 设置写入完成回调（发送缓冲区清空的回调）。不是线程安全的
  void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writeCompleteCallback_ = cb;
  }

 private:
  // 处理新连接的函数，非线程安全但在事件循环中调用
  void newConnection(int sockfd, const InetAddress& peerAddr);
  // 从 TcpServer 的连接映射中移除指定的 TcpConnection 对象
  void removeConnection(const TcpConnectionPtr& conn);

  // 定义一个连接映射，用于存储已建立连接的TcpConnection对象
  typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

  EventLoop *loop_;                               // TcpServer所属的事件循环
  const std::string name_;                        // 服务器的名称
  std::unique_ptr<Acceptor> acceptor_;            // 避免直接暴露Acceptor对象，使用Acceptor来获得新连接的fd。
  ConnectionCallback connectionCallback_;         // 连接回调函数
  MessageCallback messageCallback_;               // 消息回调函数
  WriteCompleteCallback writeCompleteCallback_;   // 写入完成回调（发送缓冲区清空的回调）
  bool started_;                                  // 服务器是否已启动标志
  int nextConnId_;                                // 下一个连接的ID，始终在事件循环线程中访问
  ConnectionMap connections_;                     // 存储已建立连接的映射
};

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_TCPSERVER_