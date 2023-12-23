#ifndef CSERVER_NET_INCLUDE_CALLBACKS_
#define CSERVER_NET_INCLUDE_CALLBACKS_

#include <functional>
#include <memory>
#include "Timestamp.h"

namespace cServer {

// 所有客户端可见的回调函数都定义在这里。

class Buffer;

class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

// 定义Timer回调函数类型
typedef std::function<void()> TimerCallback;

// 定义TcpConnection回调函数
typedef std::function<void(const TcpConnectionPtr &)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr &, Buffer *buf, Timestamp)> MessageCallback;
typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;  // 高水位回调（即发送缓冲区数据过多），如果输出缓冲区长度超过用户指定大小，就会触发回调（只在上升沿触发）
// 连接关闭时的回调函数类型
typedef std::function<void(const TcpConnectionPtr &)> CloseCallback;

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_CALLBACKS_