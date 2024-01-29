#ifndef CSERVER_NET_INCLUDE_EPOLLER_
#define CSERVER_NET_INCLUDE_EPOLLER_

#include <map>
#include <vector>
#include "Timestamp.h"
#include "EventLoop.h"
#include "noncopyable.h"

struct epoll_event;   // epoll事件结构体的前向声明

namespace cServer {

class Channel;

///
/// IO Multiplexing with epoll(4).
///
/// This class doesn't own the Channel objects.
/// 使用epoll进行IO多路复用。
/// 该类不拥有Channel对象。
class EPoller : noncopyable {
 public:
  typedef std::vector<Channel*> ChannelList;  // Channel列表类型定义

  EPoller(EventLoop* loop);
  ~EPoller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  /// 轮询IO事件。
  /// 必须在事件循环线程中调用。
  Timestamp poll(int timeoutMs, ChannelList* activeChannels);

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  /// 更改感兴趣的IO事件。
  /// 必须在事件循环线程中调用。
  void updateChannel(Channel* channel);
  /// Remove the channel, when it destructs.
  /// Must be called in the loop thread.
  /// 当Channel析构时删除通道。
  /// 必须在事件循环线程中调用。
  void removeChannel(Channel* channel);

  // 断言当前线程为事件循环所在线程
  void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }

 private:
  static const int kInitEventListSize = 16;  // epoll事件列表初始大小

  // 填充活动Channel列表
  void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
  // 更新Channel的IO事件
  void update(int operation, Channel* channel);

  typedef std::vector<struct epoll_event> EventList;  // epoll事件列表类型定义
  typedef std::map<int, Channel*> ChannelMap;  // Channel映射类型定义

  EventLoop* ownerLoop_;  // 拥有该EPoller的事件循环指针
  int epollfd_;           // epoll文件描述符
  EventList events_;      // epoll事件列表
  ChannelMap channels_;   // Channel映射，用于快速查找通道
};

}  // namespace cServer
#endif  // CSERVER_NET_INCLUDE_EPOLLER_
