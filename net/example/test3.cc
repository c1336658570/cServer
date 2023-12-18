// g++ test3.cc ../src/* ../../tool/src/* ../../log/src/*  -I ../include -I ../../tool/include -I ../../log/include/
#include <stdio.h>
#include <string.h>
#include <sys/timerfd.h>
#include "Channel.h"
#include "EventLoop.h"

cServer::EventLoop *g_loop;  // 全局指针，用于在timeout中通知事件循环退出

// poll(2)是level trigger，在timeout()中应该read() timefd，否则下次会立刻触发。
void timeout(cServer::Timestamp receiveTime) {
  printf("%s Timeout!\n", receiveTime.toFormatString().c_str());
  g_loop->quit();  // 在超时时通知事件循环退出
}

int main() {
  printf("%s started\n", cServer::Timestamp::now().toFormatString().c_str());
  cServer::EventLoop loop;  // 创建事件循环对象
  g_loop = &loop;  // 将全局指针指向事件循环对象，以便在timeout函数中使用

  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);  // 创建定时器文件描述符
  cServer::Channel channel(&loop, timerfd);  // 创建事件通道，关联到事件循环和定时器文件描述符
  channel.setReadCallback(timeout);  // 设置读事件回调函数为timeout
  channel.enableReading();           // 启用读事件监听

  struct itimerspec howlong;
  memset(&howlong, 0, sizeof(howlong));
  howlong.it_value.tv_sec = 5;  // 设置定时器超时时间为5秒
  ::timerfd_settime(timerfd, 0, &howlong, NULL);  // 设置定时器参数

  loop.loop();

  ::close(timerfd);
}
