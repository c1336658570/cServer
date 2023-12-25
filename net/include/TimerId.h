#ifndef CSERVER_NET_INCLUDE_TIMERID_
#define CSERVER_NET_INCLUDE_TIMERID_

namespace cServer {

// Timer类的前向声明，声明TimerId类用于标识定时器
class Timer;

// TimerId类，用于表示定时器的不透明标识符，用于取消定时器
class TimerId {
 public:
  TimerId(Timer* timer = NULL, int64_t seq = 0) : timer_(timer), seq_(seq) {
  }

  // 默认的拷贝构造函数、析构函数和赋值运算符是可以的

  friend class TimerQueue;

 private:
  Timer* timer_;    // 定时器
  int64_t seq_;     // 序列号
};

}  // namespace cServer

#endif  // CSERVER_NET_INCLUDE_TIMERID_