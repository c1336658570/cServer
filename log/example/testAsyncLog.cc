// g++ testAsyncLog.cc ../src/* ../../tool/src/* -I ../include -I ../../tool/include
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "AsyncLogging.h"
#include "Logging.h"

int ROLL_SIZE = 500 * 1000 * 1000;
std::unique_ptr<cServer::AsyncLogging> logging;

void asyncOut(const char *msg, int len) {
  logging->append(msg, len);
}

int main(int argc, char *argv[]) {
  logging = std::make_unique<cServer::AsyncLogging>(::basename(argv[0]), ROLL_SIZE);
  logging->start();

  cServer::Logger::setOutput(asyncOut);
  // 返回当前时刻的时间点，使用稳定时钟 (steady_clock)，它是一个不受系统时钟调整影响的时钟。
  auto now = std::chrono::steady_clock::now();
  for (int i = 0; i < 1000000; ++i) {
    LOG_INFO << "Hello 12312313";
  }
  // 返回当前时刻的时间点，使用稳定时钟 (steady_clock)，它是一个不受系统时钟调整影响的时钟。
  auto end = std::chrono::steady_clock::now();
  // 计算两个时间点之间的时间间隔，得到一个 std::chrono::duration 对象。
  // std::chrono::duration_cast<std::chrono::milliseconds>(...) 将这个时间间隔转换为毫秒表示。
  auto t = std::chrono::duration_cast<std::chrono::milliseconds>(end - now);
  std::cout << "write 1000000 notes, spend: " << t.count() << "ms" << std::endl;
  // 用于让当前线程休眠
  // std::chrono::seconds(5) 创建了一个表示5秒的时间段。std::chrono::seconds
  // 是 std::chrono 命名空间中的一个类型，用于表示以秒为单位的时间段。
  // 将这个时间段作为参数传递给 std::this_thread::sleep_for，使得当前线程休眠5秒钟。
  std::this_thread::sleep_for(std::chrono::seconds(5));
}