#ifndef CSERVER_NET_INCLUDE_POLLER_
#define CSERVER_NET_INCLUDE_POLLER_

#include <map>
#include <vector>
#include "Timestamp.h"
#include "EventLoop.h"

struct pollfd;    // 前向声明了struct pollfd

namespace cServer {

class Channel;    // 前向声明了class Channel

/*
 * Poller是EventLoop的间接成员，只供其owner EventLoop在IO线程调用，因此无须加锁。
 * 其生命期与EventLoop相等。Poller并不拥有Channel，
 * Channel在析构之前必须自己unregister（EventLoop::removeChannel()），避免空悬指针。
 */
class Poller : noncopyable {
 public:
  typedef std::vector<Channel *> ChannelList;

  // 构造函数：用指定的EventLoop初始化Poller。loop是此poller所属的EventLoop
  Poller(EventLoop *loop);
  ~Poller();

  // 使用指定的超时时间轮询I/O事件，并将准备就绪的通道填充到activeChannels中
  Timestamp poll(int timeoutMs, ChannelList *activeChannels);

  void updateChannel(Channel *channel);   // 更新channel的监听状态

  // 从 EventLoop 中移除指定的 Channel 对象。
  // 该函数用于在 Channel 对象析构时调用，必须在 EventLoop 线程中调用。
  void removeChannel(Channel* channel);

  // 检查调用线程是否是所有者EventLoop的线程。
  void assertInLoopThread() {
    ownerLoop_->assertInLoopThread();
  }

 private:
  void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

  typedef std::vector<struct pollfd> PollFdList;
  typedef std::map<int, Channel *> ChannelMap;     // fd到Channel*的映射

  EventLoop *ownerLoop_;    // 此poller所属的EventLoop
  PollFdList pollfds_;      // Poller::poll()不会在每次调用poll(2)之前临时构造pollfd数组，而是把它缓存起来（pollfds_）。
  ChannelMap channels_;     // fd到Channel*的映射

};

} // namespace cServer

#endif  // CSERVER_NET_INCLUDE_POLLER_