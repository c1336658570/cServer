#include <unistd.h>
#include <netinet/tcp.h>
#include "Socket.h"
#include "Logging.h"

namespace cServer {

// 创建一个非阻塞套接字
int createNonblocking() {
  // 设置为非阻塞和在执行时关闭
  int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd < 0) {
    LOG_SYSFATAL << "sockets::createNonblockingOrDie";
  }

  return sockfd;
}


// Socket对象的析构函数，用于关闭套接字。
Socket::~Socket() {
  ::close(sockfd_);
}

// 绑定套接字到指定的本地地址。
void Socket::bindAddress(const InetAddress& addr) {
  int ret = ::bind(sockfd_, (struct sockaddr*)(&addr.getSockAddrInet()), sizeof(struct sockaddr_in));
  if (ret < 0) {
    LOG_SYSFATAL << "Socket::bindAddress";
  }
}

// 监听连接请求。
void Socket::listen() {
  int ret = ::listen(sockfd_, SOMAXCONN);
  if (ret < 0) {
    LOG_SYSFATAL << "Socket::listen";
  }
}

// 接受客户端的连接请求，返回新的已连接套接字的文件描述符，并获取客户端地址。
int Socket::accept(InetAddress* peerAddr) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  socklen_t addrlen = sizeof(addr);

  int connfd = ::accept4(sockfd_, (struct sockaddr*)&addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd >= 0) {
    peerAddr->setSockAddrInet(addr);
  } else {
    int savedErrno = errno;
    LOG_FATAL << "Socket::accept";
  }

  return connfd;
}

// 设置SO_REUSEADDR选项，允许重用本地地址。
void Socket::setReuseAddr(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

// 用于设置TCP_NODELAY选项，启用或禁用Nagle算法
void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

// 设置TCP_KEEPALIVE选项，启用或禁用TCP的keep-alive机制
void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

// 设置SO_REUSEPORT选项，允许多个套接字同时绑定到相同的端口
void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT   // 检查是否支持SO_REUSEPORT选项。
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
  if (ret < 0 && on) {
    LOG_SYSERR << "Socket::setReusePort SO_REUSEPORT failed.";
  }
#else
  if (on) {
    LOG_ERROR << "Socket::setReusePort SO_REUSEPORT is not supported.";  // 不支持SO_REUSEPORT
  }
#endif
}

// 用于关闭套接字的写入功能（半关闭）
void Socket::shutdownWrite() {
  if (::shutdown(sockfd_, SHUT_WR) < 0) {
    LOG_ERROR << "Socket::shutdownWrite";
  }
}

}  // namespace cServer