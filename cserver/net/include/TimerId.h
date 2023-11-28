#ifndef CSERVER_NET_INCLUDE_TIMERID_
#define CSERVER_NET_INCLUDE_TIMERID_

namespace cServer {

// Timer类的前向声明，声明TimerId类用于标识定时器
class Timer;

// TimerId类，用于表示定时器的不透明标识符，用于取消定时器
class TimerId {
 public:
  // 显式构造函数，接受一个Timer指针作为参数，用于创建TimerId对象
  explicit TimerId(Timer *timer) : value_(timer) {
  }

 private:
  Timer *value_;
};

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_TIMERID_