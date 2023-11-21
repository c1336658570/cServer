#include "Timestamp.h"
#include <sys/time.h>
#include <cinttypes>

namespace cServer {

// 将时间戳转换为字符串表示
std::string Timestamp::toString() const {
  char buf[32] = {0};
  // 将微秒数转换为秒数和微秒数
  auto seconds = static_cast<time_t>(microsecondsSinceEpoch_ / MicrosecondsPerSecond);
  int64_t microseconds = static_cast<int>(microsecondsSinceEpoch_ % MicrosecondsPerSecond);
  // 格式化输出秒数和微秒数
  // "%" PRId64 ".%06" PRId64 "" 是格式化字符串，包含了两个占位符。PRId64 是表示带符号 64 位整数的宏。
  snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
  return buf;
}

// 将时间戳转换为格式化的字符串表示
std::string Timestamp::toFormatString() const {
  char buf[32] = {0};
  // 将微秒数转换为秒数和微秒数
  auto seconds = static_cast<time_t>(microsecondsSinceEpoch_ / MicrosecondsPerSecond);
  int microseconds = static_cast<int>(microsecondsSinceEpoch_ % MicrosecondsPerSecond);
  struct tm t;

  // 将秒数转换为UTC时间结构
  gmtime_r(&seconds, &t);

  // 格式化输出年月日时分秒和微秒
  snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d.%06d",
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min,
           t.tm_sec, microseconds);
  return buf;
}

// 获取当前时间戳
Timestamp Timestamp::now() {
  struct timeval tv;
  // 获取当前时间戳（秒和微秒）
  gettimeofday(&tv, nullptr);
  return Timestamp(tv.tv_sec * MicrosecondsPerSecond + tv.tv_usec);
}

// 返回一个无效的时间戳
Timestamp Timestamp::invalid() { return Timestamp(0); }

} // namespace cServer
