// g++ test4.cc ../src/* ../../tool/src/* ../../log/src/*  -I ../include -I ../../tool/include -I ../../log/include/
#include <stdio.h>
#include <functional>
#include "EventLoop.h"

int cnt = 0;
cServer::EventLoop *g_loop;

void printTid() {
  printf("pid = %d, tid = %d\n", getpid(), cServer::CurrentThread::tid());
  printf("now %s\n", cServer::Timestamp::now().toString().c_str());
}

void print(const char *msg) {
  printf("msg %s %s\n", cServer::Timestamp::now().toString().c_str(), msg);
  if (++cnt == 20) {
    g_loop->quit();
  }
}

cServer::TimerId toCancel;
void cancelSelf() {
  print("cancelSelf()");
  // 当运行到如下这行时的时候，toCancel代表的Timer已经不在timers_和activeTimers_这两个容器中，而是位于expired中
  g_loop->cancel(toCancel);
}

int main() {
  printTid();
  cServer::EventLoop loop;
  g_loop = &loop;

  print("main");
  loop.runAfter(1, std::bind(print, "once1"));
  loop.runAfter(1.5, std::bind(print, "once1.5"));
  loop.runAfter(2.5, std::bind(print, "once2.5"));
  loop.runAfter(3.5, std::bind(print, "once3.5"));
  cServer::TimerId t = loop.runEvery(2, std::bind(print, "every2"));
  loop.runEvery(3, std::bind(print, "every3"));
  loop.runAfter(10, std::bind(&cServer::EventLoop::cancel, &loop, t));
  toCancel = loop.runEvery(5, cancelSelf);    // 自注销，即在定时器回调中取消定时器

  loop.loop();
  print("main loop exits");
  sleep(1);
}
