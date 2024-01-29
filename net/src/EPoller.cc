
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include "EPoller.h"
#include "Channel.h"
#include "Logging.h"

using namespace cServer;

namespace {
const int kNew = -1;     // 表示新通道
const int kAdded = 1;    // 表示已添加到EPoll中
const int kDeleted = 2;  // 表示已从EPoll中删除
}  // namespace

// EPoller类的构造函数
EPoller::EPoller(EventLoop* loop)
    : ownerLoop_(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),   // 创建一个epoll实例
      events_(kInitEventListSize) {               // 初始化事件列表
  if (epollfd_ < 0) {  // 如果创建epoll失败
    LOG_SYSFATAL << "EPoller::EPoller";
  }
}

EPoller::~EPoller() {
  ::close(epollfd_);  // 关闭epoll文件描述符
}

// 对文件描述符进行轮询
Timestamp EPoller::poll(int timeoutMs, ChannelList* activeChannels) {
  int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),
                               static_cast<int>(events_.size()),
                               timeoutMs);  // 进行epoll_wait调用，等待事件发生
  Timestamp now(Timestamp::now());                  // 获取当前时间戳
  if (numEvents > 0) {                              // 如果有事件发生
    LOG_TRACE << numEvents << " events happended";  // 输出事件发生的数量
    fillActiveChannels(numEvents, activeChannels);  // 填充活动Channel列表
    if (static_cast<size_t>(numEvents) == events_.size()) {  // 如果事件数量等于事件列表大小
      events_.resize(events_.size() * 2);  // 将事件列表大小扩展为原来的两倍
    }
  } else if (numEvents == 0) {  // 如果没有事件发生
    LOG_TRACE << " nothing happended";
  } else {  // 如果发生了错误
    LOG_SYSERR << "EPoller::poll()";
  }
  return now;  // 返回当前时间戳
}

// 填充活动Channel列表
void EPoller::fillActiveChannels(int numEvents,
                                 ChannelList* activeChannels) const {
  assert(static_cast<size_t>(numEvents) <= events_.size());  // 确保事件数量不超过事件列表大小
  for (int i = 0; i < numEvents; ++i) {
    Channel* channel = static_cast<Channel*>(events_[i].data.ptr);  // 获取Channel指针
    channel->set_revents(events_[i].events);  // 设置Channel关联的事件类型
    activeChannels->push_back(channel);  // 将Channel加入到活动通道列表中
  }
}

// 更新Channel
void EPoller::updateChannel(Channel* channel) {
  assertInLoopThread();  // 确保在事件循环线程中调用该函数
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();  // 输出日志
  const int index = channel->index();              // 获取Channel的索引
  if (index == kNew || index == kDeleted) {   // 如果Channel是新创建的或者已经被删除的
    // 新的Channel，使用EPOLL_CTL_ADD添加到EPoll中
    int fd = channel->fd(); // 获取文件描述符
    if (index == kNew) {    // 如果是新的Channel
      assert(channels_.find(fd) == channels_.end());  // 确保Channel映射中不存在该文件描述符
      channels_[fd] = channel;  // 将Channel加入到Channel映射中
    } else {                    // 如果是已经被删除的Channel
      assert(channels_.find(fd) != channels_.end());  // 确保Channel映射中存在该文件描述符
      assert(channels_[fd] == channel);  // 确保Channel映射中文件描述符对应的Channel与当前Channel一致
    }
    channel->set_index(kAdded);  // 设置Channel的索引为已添加到EPoll中
    update(EPOLL_CTL_ADD, channel);  // 调用EPoll的控制函数进行添加操作
  } else {                           // 如果Channel是已存在的
    // 使用EPOLL_CTL_MOD/DEL更新已存在的Channel
    int fd = channel->fd();  // 获取文件描述符
    (void)fd;
    assert(channels_.find(fd) != channels_.end());  // 确保Channel映射中存在该文件描述符
    assert(channels_[fd] == channel);  // 确保Channel映射中文件描述符对应的Channel与当前Channel一致
    assert(index == kAdded);  // 确保通道的索引为已添加到EPoll中
    if (channel->isNoneEvent()) {  // 如果Channel不关注任何事件
      update(EPOLL_CTL_DEL, channel);  // 调用EPoll的控制函数进行删除操作
      channel->set_index(kDeleted);  // 设置Channel的索引为已从EPoll中删除
    } else {                         // 如果Channel关注事件
      update(EPOLL_CTL_MOD, channel);  // 调用EPoll的控制函数进行修改操作
    }
  }
}

// 删除Channel
void EPoller::removeChannel(Channel* channel) {
  assertInLoopThread();
  int fd = channel->fd();  // 获取文件描述符
  LOG_TRACE << "fd = " << fd;
  assert(channels_.find(fd) != channels_.end());  // 确保Channel映射中存在该文件描述符
  assert(channels_[fd] == channel);  // 确保Channel映射中文件描述符对应的Channel与当前Channel一致
  assert(channel->isNoneEvent());  // 确保Channel不关注任何事件
  int index = channel->index();    // 获取Channel的索引
  assert(index == kAdded || index == kDeleted);  // 确保Channel的索引为已添加到EPoll中或已从EPoll中删除
  size_t n = channels_.erase(fd);  // 从Channel映射中删除该文件描述符对应的Channel
  (void)n;
  assert(n == 1);  // 确保删除的Channel数量为1

  if (index == kAdded) {  // 如果Channel是已添加到EPoll中的
    update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);  // 设置Channel的索引为新创建
}

// 更新Channel的事件
void EPoller::update(int operation, Channel* channel) {
  struct epoll_event event;  // 创建epoll事件结构体
  bzero(&event, sizeof event);
  event.events = channel->events();  // 设置事件类型
  event.data.ptr = channel;          // 设置数据指针为Channel指针
  int fd = channel->fd();            // 获取文件描述符
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) { // 调用EPoll的控制函数进行操作
    if (operation == EPOLL_CTL_DEL) {  // 如果是删除操作
      LOG_SYSERR << "epoll_ctl op=" << operation << " fd=" << fd;
    } else {  // 如果是其他操作
      LOG_SYSFATAL << "epoll_ctl op=" << operation << " fd=" << fd;
    }
  }
}
