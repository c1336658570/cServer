#ifndef CSERVER_NET_INCLUDE_CHANNEL_
#define CSERVER_NET_INCLUDE_CHANNEL_

#include <functional>
#include "noncopyable.h"

namespace cServer {

class EventLoop;

/*
 * 每个Channel对象自始至终只属于一个EventLoop，因此每个Channel对象都只属于某一个IO线程。
 * 每个Channel对象自始至终只负责一个文件描述符（fd）的IO事件分发，但它并不拥有这个fd，
 * 也不会在析构的时候关闭这个fd。Channel会把不同的IO事件分发为不同的回调，
 * 例如ReadCallback、WriteCallback等，而且“回调”用std::function表示，用户无须继承Channel，
 * Channel不是基类。Channel的生命期由其owner class负责管理，它一般是其他class的直接或间接成员。
 */
class Channel : noncopyable {
 public:
  typedef std::function<void()> EventCallback;    // 定义回调函数类型

  Channel(EventLoop *loop, int fd);   // 构造函数，所属的EventLoop和channel所管理的fd
  ~Channel();                         // 析构函数


  // 处理事件的函数  
  void handleEvent();
  // 设置读、写、异常回调函数
  void setReadCallback(const EventCallback &cb) {
    readCallback_ = cb;
  }
  void setWriteCallback(const EventCallback &cb) {
    writeCallback_ = cb;
  }
  void setErrorCallback(const EventCallback &cb) {
    errorCallback_ = cb;
  }
  // 设置关闭事件回调函数
  void setCloseCallback(const EventCallback &cb) {
    closeCallback_ = cb;
  }

  // 获取文件描述符
  int fd() const {
    return fd_;
  }
  // 获取监听的事件类型
  int events() const {
    return events_;
  }
  // 设置活动的事件类型
  void set_revents(int revt) {
    revents_ = revt;
  }
  // 判断是否监听事件
  bool isNoneEvent() const {
    return events_ == kNoneEvent;
  }

  // 启用读事件
  void enableReading() {
    events_ |= kReadEvent;
    update();   // 调用update将当前这个channel加入到poller_的ChannelMap
  }
  // void enableWriting() { events_ |= kWriteEvent; update(); }
  // void disableWriting() { events_ &= ~kWriteEvent; update(); }
  // 禁用所有关注的事件
  void disableAll() {
    events_ = kNoneEvent;
    update();
  }

  // 获取在Poller中的索引
  int index() {
    return index_;
  }

  // 设置在Poller中的索引
  void set_index(int idx) {
    index_ = idx;
  }

  // 获取所属的EventLoop对象
  EventLoop *ownerLoop() {
    return loop_;
  }

 private:
  // 更新Channel的关注事件
  void update();

  static const int kNoneEvent;    // 不监听事件的标记
  static const int kReadEvent;    // 监听读的标记
  static const int kWriteEvent;   // 监听写的标记

  EventLoop *loop_; // 所属的EventLoop
  const int fd_;    // channel管理的文件描述符
  int events_;      // 它关心的IO事件，由用户设置
  int revents_;     // 目前活动的事件，由EventLoop/poller设置
  int index_;       // used by Poller.

  bool eventHandling_;    // 是否正在处理事件

  EventCallback readCallback_;    // 读回调
  EventCallback writeCallback_;   // 写回调
  EventCallback errorCallback_;   // 异常回调
  EventCallback closeCallback_;   // 关闭连接回调
};

}  // namespace cServer

#endif  // CSERVER_NET_INCLUDE_CHANNEL_