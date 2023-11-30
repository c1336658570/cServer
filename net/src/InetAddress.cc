#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "InetAddress.h"
#include "Logging.h"

namespace cServer {

/*
 * 头文件：#include <endian.h>
 * (无符号)64字节主机转网络：htobe64(uint64_t data)
 * (无符号)64字节网络转主机：be64toh(uint64_t data)
 * (无符号)32字节主机转网络：htobe32(uint32_t data)
 * (无符号)32字节网络转主机：be32toh(uint32_t data)
 * (无符号)16字节主机转网络：htobe16(uint16_t data)
 * (无符号)16字节网络转主机：be16toh(uint16_t data)
 */

InetAddress::InetAddress(uint16_t port) {
  memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = htonl(INADDR_ANY);
  addr_.sin_port = htons(port);
}

InetAddress::InetAddress(const std::string &ip, uint16_t port) {
  memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  if (inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
    LOG_SYSERR << "inet_pton";
  }
  addr_.sin_port = port;
}

std::string InetAddress::toHostPort() const {
  char buf[32];
  char host[INET_ADDRSTRLEN] = "INVALID";
  ::inet_ntop(AF_INET, &addr_.sin_addr, host, sizeof(host));
  uint16_t port = ntohs(addr_.sin_port);
  snprintf(buf, sizeof(buf), "%s:%u", host, port);

  return buf;
}

}
