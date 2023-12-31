#include <poll.h>
#include <cassert>
#include "Poller.h"
#include "Channel.h"
#include "noncopyable.h"
#include "Logging.h"

namespace cServer {

Poller::Poller(EventLoop* loop) : ownerLoop_(loop) {
}

Poller::~Poller() {
}

// 调用poll(2)获得当前活动的IO事件，然后填充调用方传入的activeChannels，并返回poll(2) return的时刻。
Timestamp Poller::poll(int timeoutMs, ChannelList *activeChannels) {
  // 调用poll(2)获得当前活动的IO事件，然后填充调用方传入的activeChannels
  int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs); // 第一个参数可写为pollfds.data()
  Timestamp now(Timestamp::now());
  if (numEvents > 0) {
    LOG_TRACE << numEvents << " events happended";
    fillActiveChannels(numEvents, activeChannels);    // 填充调用方传入的activeChannels
  } else if (numEvents == 0) {
    LOG_TRACE << " nothing happended";
  } else {
    LOG_SYSERR << "Poller::poll()";
  }
  return now;     // 返回poll(2) return的时刻。
}

// 遍历pollfds_，找出有活动事件的fd，把它对应的Channel填入activeChannels。
// 函数的复杂度是O(N)，其中N是pollfds_的长度，即文件描述符数目。
void Poller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
  for (PollFdList::const_iterator pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents > 0; ++pfd) {
    if (pfd->revents > 0) {
      // 为了提前结束循环，每找到一个活动fd就递减numEvents，当numEvents减为0时表示活动fd都找完了。
      --numEvents;
      ChannelMap::const_iterator ch = channels_.find(pfd->fd);
      assert(ch != channels_.end());
      Channel *channel = ch->second;
      assert(channel->fd() == pfd->fd);
      // 当前活动事件revents会保存在Channel中，供Channel::handleEvent()使用
      channel->set_revents(pfd->revents);
      // pfd->revents = 0;
      activeChannels->push_back(channel);
    }
  }
}

// 负责维护和更新pollfds_数组。
// 添加新Channel的复杂度是O(logN)，更新已有的Channel的复杂度是O(1)，
// 因为Channel记住了自己在pollfds_数组中的下标，因此可以快速定位。removeChannel()的复杂度也将会是O(logN)。
void Poller::updateChannel(Channel *channel) {
  assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
  if (channel->index() < 0) {
    // a new one, add to pollfds_
    assert(channels_.find(channel->fd()) == channels_.end());
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);
    int idx = static_cast<int>(pollfds_.size()) - 1;
    channel->set_index(idx);
    channels_[pfd.fd] = channel;
  } else {
    // update existing one
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    struct pollfd& pfd = pollfds_[idx];
    assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    if (channel->isNoneEvent()) {
      // ignore this pollfd
      // 如果某个Channel暂时不关心任何事件，就把pollfd.fd设为-1，让poll(2)忽略此项
      // 不能改为把pollfd.events设为0，这样无法屏蔽POLLER事件。
      // 改进的做法是把pollfd.fd设为channel->fd()的相反数减一，这样可以进一步检查invariant。
      // pfd.fd = -1;
      pfd.fd = -channel->fd()-1;
    }
  }
}

// 从 Poller 中移除指定的 Channel 对象
void Poller::removeChannel(Channel *channel) {
  assertInLoopThread();
  // 记录日志，标明正在移除的文件描述符（fd）
  LOG_TRACE << "fd = " << channel->fd();
  // 断言确保要移除的 Channel 在 channels_ 中存在
  assert(channels_.find(channel->fd()) != channels_.end());
  // 断言确保在 channels_ 中找到的 Channel 就是要移除的 Channel
  assert(channels_[channel->fd()] == channel);
  // 断言确保 Channel 的事件状态为 kNoneEvent
  assert(channel->isNoneEvent());
  // 获取要移除的 Channel 在 pollfds_ 中的索引
  int idx = channel->index();
  // 断言确保索引值在合法范围内
  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
  // 获取指定索引处的 pollfd 结构体
  const struct pollfd &pfd = pollfds_[idx]; (void)pfd;
  // 断言确保该 pollfd 结构体与 Channel 的状态一致
  assert(pfd.fd == -channel->fd()-1 && pfd.events == channel->events());
  // 从 channels_ 中移除指定的 Channel
  size_t n = channels_.erase(channel->fd());
  // 断言确保移除的 Channel 数量为 1
  assert(n == 1); (void)n;
  // 如果要移除的 Channel 在 pollfds_ 中的索引是最后一个，则直接弹出
  if (static_cast<size_t>(idx) == pollfds_.size()-1) {
    pollfds_.pop_back();
  } else {
    // 否则，将最后一个 pollfd 移动到要移除的位置
    int channelAtEnd = pollfds_.back().fd;
    iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
    if (channelAtEnd < 0) {
      channelAtEnd = -channelAtEnd-1;
    }
    // 更新最后一个 Channel 在 channels_ 中的索引
    channels_[channelAtEnd]->set_index(idx);
    // 弹出最后一个 pollfd
    pollfds_.pop_back();
  }
}


}