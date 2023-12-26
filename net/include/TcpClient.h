#ifndef CSERVER_NET_INCLUDE_TCPCLIENT_
#define CSERVER_NET_INCLUDE_TCPCLIENT_

#include "Mutex.h"
#include "TcpConnection.h"
#include "noncopyable.h"

namespace cServer {

/*
 * 每个TcpClient只管理一个TcpConnection。
 * 
 * TcpClient具备TcpConnection断开之后重新连接的功能，加上Connector具备反复尝试连接的功能，
 * 因此客户端和服务端的启动顺序无关紧要。可以先启动客户端，一旦服务端启动，
 * 半分钟之内即可恢复连接（由Connector:: kMaxRetryDelayMs常数控制）；在客户端运行期间服务端可以重启，客户端也会自动重连。
 * 
 * 连接断开后初次重试的延迟应该有随机性，比方说服务端崩溃，它所有的客户连接同时断开，
 * 然后0.5s之后同时再次发起连接，这样既可能造成SYN丢包，也可能给服务端带来短期大负载，
 * 影响其服务质量。因此每个TcpClient应该等待一段随机的时间（0.5～2s），再重试，避免拥塞。
 * 
 * 发起连接的时候如果发生TCP SYN丢包，那么系统默认的重试间隔是3s，这期间不会返回错误码，而且这个间隔似乎不容易修改。
 * 如果需要缩短间隔，可以再用一个定时器，在0.5s或1s之后发起另一次连接。如果有需求的话，这个功能可以做到Connector中。
 */

class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient : noncopyable {
 public:
  // 构造函数，初始化TcpClient对象
  TcpClient(EventLoop *loop, const InetAddress &serverAddr);
  // 析构函数，销毁TcpClient对象
  ~TcpClient();

  void connect();       // 连接到服务器
  void disconnect();    // 断开连接
  void stop();          // 停止客户端

  // 返回当前的连接对象
  TcpConnectionPtr connection() const {
    MutexLockGuard lock(mutex_);
    return connection_;
  }

  // 返回是否支持重连
  bool retry() const;
  // 开启重连功能
  void enableRetry() { retry_ = true; }

  // 设置连接建立时的回调函数，非线程安全
  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }

  // 设置消息到达时的回调函数，非线程安全
  void setMessageCallback(const MessageCallback &cb) {
    messageCallback_ = cb;
  }

  // 设置数据发送完成时的回调函数，非线程安全
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }

 private:
  // 建立新的连接，非线程安全，但在事件循环线程中调用
  void newConnection(int sockfd);
  // 移除连接，非线程安全，但在事件循环线程中调用
  void removeConnection(const TcpConnectionPtr &conn);

  EventLoop *loop_;           // EventLoop对象，用于事件驱动
  ConnectorPtr connector_;    // 连接器，用于建立连接
  ConnectionCallback connectionCallback_;     // 连接建立时的回调函数
  MessageCallback messageCallback_;           // 消息到达时的回调函数
  WriteCompleteCallback writeCompleteCallback_;   // 数据发送完成时的回调函数
  bool retry_;          // 是否支持重连，原子操作
  bool connect_;        // 是否已连接，原子操作
  // 总在loop thread中
  int nextConnId_;      // 下一个连接的ID
  mutable MutexLock mutex_;       // 互斥锁，用于保护连接对象的访问
  TcpConnectionPtr connection_;   // 当前的连接对象，受mutex_保护
};

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_TCPCLIENT_
