#ifndef CSERVER_TOOL_INCLUDE_TIMESTAMP_
#define CSERVER_TOOL_INCLUDE_TIMESTAMP_
#include <cstdint>
#include <string>

namespace cServer {

// 时间戳类，用于表示时间点
class Timestamp {
 public:
  // 默认构造函数，创建一个无效的时间戳对象
  Timestamp() : microsecondsSinceEpoch_(0) {}

  // 显式构造函数，创建一个指定微秒数的时间戳对象
  explicit Timestamp(int64_t microseconds)
      : microsecondsSinceEpoch_(microseconds) {}

  // 将时间戳转换为字符串表示
  std::string toString() const;

  // 将时间戳转换为格式化的字符串表示
  std::string toFormatString() const;

  // 判断时间戳是否有效
  bool isValid() const { return microsecondsSinceEpoch_ > 0; }

  // 获取时间戳的微秒数  
  int64_t microsecondsSinceEpoch() const { return microsecondsSinceEpoch_; }

  // 获取当前时间戳
  static Timestamp now();

  // 获取一个无效的时间戳
  static Timestamp invalid();

  // 定义每秒的微秒数
  static const int MicrosecondsPerSecond = 1000 * 1000;

 private:
  int64_t microsecondsSinceEpoch_;    // 存储时间戳的微秒数
};

// 重载 < 运算符，用于比较两个时间戳的大小
inline bool operator<(Timestamp lhs, Timestamp rhs) {
  return lhs.microsecondsSinceEpoch() < rhs.microsecondsSinceEpoch();
}

// 重载 == 运算符，用于判断两个时间戳是否相等
inline bool operator==(Timestamp lhs, Timestamp rhs) {
  return lhs.microsecondsSinceEpoch() == rhs.microsecondsSinceEpoch();
}

// 将时间戳增加指定秒数，并返回新的时间戳
inline Timestamp addTime(Timestamp timestamp, double seconds) {
  auto delta = static_cast<int64_t>(seconds * Timestamp::MicrosecondsPerSecond);
  return Timestamp(timestamp.microsecondsSinceEpoch() + delta);
}

}  // namespace cServer
#endif  // CSERVER_TOOL_INCLUDE_TIMESTAMP_
