#ifndef CSERVER_NET_INCLUDE_SOCKET_
#define CSERVER_NET_INCLUDE_SOCKET_

#include "noncopyable.h"
#include "InetAddress.h"

namespace cServer {

// 创建一个非阻塞套接字
int createNonblocking();
// 获取套接字的相关错误状态
int getSocketError(int sockfd);

class Socket : noncopyable {
 public:
  // 使用给定的文件描述符构造Socket对象。
  explicit Socket(int sockfd) : sockfd_(sockfd) {
  }

  // Socket 对象的析构函数，用于关闭套接字。
  ~Socket();

  // 返回套接字的文件描述符。
  int fd() const {
    return sockfd_;
  }

  // 绑定套接字到指定的本地地址。
  void bindAddress(const InetAddress& localaddr);
  // 监听连接请求。
  void listen();
  // 接受客户端的连接请求，返回新的已连接套接字的文件描述符，并获取客户端地址。
  int accept(InetAddress* peeraddr);
  // 设置SO_REUSEADDR选项，允许重用本地地址。
  void setReuseAddr(bool on);
  // 用于设置TCP_NODELAY选项，启用或禁用Nagle算法
  void setTcpNoDelay(bool on); 
  // 设置TCP_KEEPALIVE选项，启用或禁用TCP的keep-alive机制
  void setKeepAlive(bool on);
  // 设置SO_REUSEPORT选项，允许多个套接字同时绑定到相同的端口
  void setReusePort(bool on);
  // 用于关闭套接字的写入功能（半关闭）
  void shutdownWrite();

 private:
  const int sockfd_;
};

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_SOCKET_