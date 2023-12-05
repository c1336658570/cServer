#ifndef CSERVER_NET_INCLUDE_CALLBACKS_
#define CSERVER_NET_INCLUDE_CALLBACKS_

#include <functional>
#include <memory>

namespace cServer {

// 所有客户端可见的回调函数都定义在这里。

class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

// 定义Timer回调函数类型
typedef std::function<void()> TimerCallback;

// 定义TcpConnection回调函数
typedef std::function<void(const TcpConnectionPtr &)> ConnectionCallback;
typedef std::function<void (const TcpConnectionPtr &, const char *data, ssize_t len)> MessageCallback;

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_CALLBACKS_