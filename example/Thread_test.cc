//  g++ Thread_test.cc ../tool/src/* ../log/src/* -I ../tool/include -I ../log/include
#include <functional>
#include "../tool/include/Thread.h"

void threadFunc()
{
  printf("tid=%d\n", cServer::CurrentThread::tid());
}

void threadFunc2(int x)
{
  printf("tid=%d, x=%d\n", cServer::CurrentThread::tid(), x);
}

int main(void) {
  cServer::Thread t1(threadFunc);
  t1.start();
  printf("t1.tid=%d\n", t1.tid());
  t1.join();

  cServer::Thread t2(std::bind(threadFunc2, 42));
  t2.start();
  printf("t2.tid=%d\n", t2.tid());
  t2.join();
  
  return 0;
}