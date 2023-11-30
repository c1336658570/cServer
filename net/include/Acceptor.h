#ifndef CSERVER_NET_INCLUDE_ACCEPTOR_
#define CSERVER_NET_INCLUDE_ACCEPTOR_

#include <functional>
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

namespace cServer {

class EventLoop;
class InetAddress;

class Acceptor : noncopyable {
 public:
  // 表示新连接回调函数类型，接受已连接套接字的文件描述符和对端地址作为参数。
  typedef std::function<void (int sockfd, const InetAddress &)> NewConnectionCallback;

  // 根据给定的事件循环和监听地址构造一个Acceptor对象。
  Acceptor(EventLoop *loop, const InetAddress &listenAddr);

  // 设置新连接回调函数。
  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    newConnectionCallback_ = cb;
  }

  // 返回是否正在监听连接。
  bool listenning() const { return listenning_; }
  // 监听连接请求。
  void listen();

 private:
  // 处理可读事件，表示有新的连接请求到达。
  void handleRead();

  EventLoop* loop_;           // 指向事件循环的指针。
  Socket acceptSocket_;       // 用于接受连接的套接字对象。(listenfd)
  Channel acceptChannel_;     // 用于接受连接的Channel对象。(listenfd)
  NewConnectionCallback newConnectionCallback_;   // 新连接回调函数。(conectfd)
  bool listenning_;           // 表示是否正在监听连接。
};

}

#endif