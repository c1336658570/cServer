#ifndef CSERVER_NET_INCLUDE_CALLBACKS_
#define CSERVER_NET_INCLUDE_CALLBACKS_

#include <functional>

namespace cServer {

// 所有客户端可见的回调函数都定义在这里。


// 定义Timer回调函数类型
typedef std::function<void()> TimerCallback;

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_CALLBACKS_