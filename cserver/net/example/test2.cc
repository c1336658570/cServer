// g++ test2.cc ../src/* ../../tool/src/* ../../log/src/*  -I ../include -I ../../tool/include -I ../../log/include/
#include "EventLoop.h"
#include "Thread.h"

cServer::EventLoop* g_loop;

// 在主线程创建了EventLoop对象，却试图在另一个线程调用其EventLoop::loop()，程序会因断言失效而异常终止。
void threadFunc() {
  g_loop->loop();
}

int main() {
  cServer::EventLoop loop;
  g_loop = &loop;
  cServer::Thread t(threadFunc);
  t.start();
  t.join();
}
