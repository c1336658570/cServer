#ifndef CSERVER_NET_INCLUDE_INETADDRESS_
#define CSERVER_NET_INCLUDE_INETADDRESS_

#include <string>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace cServer {

class InetAddress {
 public:
 // 根据给定的端口号构造一个网络地址对象，主要用于TcpServer监听。
  explicit InetAddress(uint16_t port);
  // 根据给定的IP地址和端口号构造一个网络地址对象。
  InetAddress(const std::string &ip, uint16_t port);
  // 根据给定的sockaddr_in结构体构造一个网络地址对象，主要用于接受新连接。
  InetAddress(const struct sockaddr_in &addr) : addr_(addr) {
  }

  // 获取网络地址的主机名和端口号，返回格式为"host:port"的字符串。
  std::string toHostPort() const;   

  // 获取sockaddr_in结构体，返回网络地址的信息。
  const struct sockaddr_in& getSockAddrInet() const {
    return addr_;
  }

  // 设置sockaddr_in结构体，用于修改网络地址的信息。
  void setSockAddrInet(const struct sockaddr_in &addr) {
    addr_ = addr;
  }

 private:
  struct sockaddr_in addr_;       // 保存网络地址的信息。
};

struct sockaddr_in getLocalAddr(int sockfd);

} // namespcae cServer

#endif  // CSERVER_NET_INCLUDE_INETADDRESS_
