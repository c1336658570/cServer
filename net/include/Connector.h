#ifndef CSERVER_NET_INCLUDE_CONNECTOR_
#define CSERVER_NET_INCLUDE_CONNECTOR_

#include "InetAddress.h"
#include "TimerId.h"

#include <memory>
#include <functional>
#include "noncopyable.h"

namespace cServer {
// 前置声明用于Connector类中的类
class Channel;
class EventLoop;
/*
 * socket是一次性的，一旦出错（比如对方拒绝连接），就无法恢复，只能关闭重来。但Connector是可以反复使用的，
 * 因此每次尝试连接都要使用新的socket文件描述符和新的Channel对象。
 * 要留意Channel对象的生命期管理，并防止socket文件描述符泄漏。
 * 错误代码与accept(2)不同，EAGAIN是真的错误，表明本机ephemeral port暂时用完，要关闭socket再延期重试。
 * “正在连接”的返回码是EINPROGRESS。另外，即便出现socket可写，也不一定意味着连接已成功建立，
 * 还需要用getsockopt(sockfd, SOL_SOCKET, SO_ERROR, ...)再次确认一下。
 * 
 * 重试的间隔应该逐渐延长，例如0.5s、1s、2s、4s，直至30s，即back-off。这会造成对象生命期管理方面的困难，
 * 如果使用EventLoop::runAfter()定时而Connector在定时器到期之前析构了怎么办？
 * 做法是在Connector的析构函数中注销定时器。
 * 
 * 要处理自连接（self-connection）。出现这种状况的原因如下。在发起连接的时候，
 * TCP/IP协议栈会先选择sourceIP和sourceport，在没有显式调用bind(2)的情况下，source IP由路由表确定，
 * source port由TCP/IP协议栈从local port range 中选取尚未使用的port（即ephemeral port）。
 * 如果destination IP正好是本机，而destination port位于local port range，且没有服务程序监听的话，
 * ephemeral port可能正好选中了destination port，这就出现(source IP, source port)
 * ＝(destination IP, destination port)的情况，即发生了自连接。
 * 处理办法是断开连接再重试，否则原本侦听destination port的服务进程也无法启动了。
 */

// Connector只负责建立socket连接，不负责创建TcpConnection，它的NewConnectionCallback回调的参数是socket文件描述符。
class Connector : noncopyable {
 public:
  // handlewrite中会调用
  typedef std::function<void(int sockfd)> NewConnectionCallback;

  // 构造函数，接受事件循环和服务器地址作为参数
  Connector(EventLoop* loop, const InetAddress& serverAddr);
  // 析构函数
  ~Connector();

  // 设置用户态的回调，handlewrite中会调用
  void setNewConnectionCallback(const NewConnectionCallback& cb) {
    newConnectionCallback_ = cb;
  }

  // 启动连接过程，可在任意线程调用
  void start();
  // 重新启动连接过程，必须在事件循环线程中调用
  void restart();
  // 停止连接过程，可在任意线程调用
  void stop();

  // 获取与此连接器关联的服务器地址
  const InetAddress& serverAddress() const {
    return serverAddr_;
  }

 private:
  // 枚举表示 Connector 的状态
  enum States {
    kDisconnected,
    kConnecting,
    kConnected
  };
  // 控制重试行为的常量
  static const int kMaxRetryDelayMs = 30 * 1000;
  static const int kInitRetryDelayMs = 500;

  // 设置 Connector 的内部状态
  void setState(States s) {
    state_ = s;
  }
  void startInLoop();               // 在事件循环中启动连接过程
  void connect();                   // 发起连接
  void connecting(int sockfd);      // 处理连接中的回调
  void handleWrite();               // 处理连接上的写入事件的回调
  void handleError();               // 处理连接中的错误事件的回调
  void retry(int sockfd);           // 使用给定的套接字文件描述符重试连接
  int removeAndResetChannel();      // 移除并重置关联的通道
  void resetChannel();              // 重置关联的通道

  EventLoop* loop_;                 // 与此连接器关联的事件循环的指针
  InetAddress serverAddr_;          // 要连接的服务器地址
  bool connect_;                    // 原子标志，指示连接是否正在进行中
  States state_;                    // Connector 的状态（已断开、正在连接、已连接）
  std::unique_ptr<Channel> channel_;    // 与此连接器关联的Channel的独占指针
  NewConnectionCallback newConnectionCallback_;   // handlewrite中会调用
  int retryDelayMs_;      // 重试延迟的毫秒数
  TimerId timerId_;       // 控制重试延迟的定时器标识符
};
typedef std::shared_ptr<Connector> ConnectorPtr;

}  // namespace cServer

#endif  // CSERVER_NET_INCLUDE_CONNECTOR_
