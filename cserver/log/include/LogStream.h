#ifndef CSERVER_LOG_INCLUDE_LOGSTREAM_
#define CSERVER_LOG_INCLUDE_LOGSTREAM_

#include "../../tool/include/noncopyable.h"
#include <cstring>
#include <string>
#include <pcre_stringpiece.h>

namespace cServer {

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template <int SIZE>
class FixedBuffer : noncopyable {
 public:
  FixedBuffer() : cur_(data_) {
  }

  ~FixedBuffer() {
  }

  // 向缓冲区追加数据，buf为源数据指针，len为数据长度
  void append(const char *buf, size_t len) {
    if (static_cast<size_t>(avail() > len)) {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  // 获取缓冲区数据的起始位置
  const char *data() const {
    return data_;
  }

  // 获取缓冲区当前数据长度
  int length() const {
    return static_cast<int>(cur_ - data_);
  }

  // 获取缓冲区当前指针位置
  char *current() {
    return cur_;
  }

  // 获取缓冲区剩余可用空间
  int avail() const {
    return static_cast<int>(end() - cur_);
  }

  // 向后移动当前指针位置
  void add(size_t len) {
    cur_ += len;
  }

  // 重置缓冲区，将当前指针重置为起始位置  
  void reset() {
    cur_ = data_;
  }

  // 将缓冲区数据清零
  void bzero() {
    memset(data_, 0, sizeof(data_));
  }

  // for used by GDB
  const char *debugString();

  // 将缓冲区数据转为字符串
  std::string toString() const {
    return string(data_, length());
  }

  // 将缓冲区数据转为StringPiece
  pcrecpp::StringPiece toStringPiece() const {
    return pcrecpp::StringPiece(data_, length());
  }


 private:
  // 返回缓冲区尾后位置
  const char *end() const {
    return data_ + sizeof(data_);
  }

  char data_[SIZE];     // 缓冲区数组
  char *cur_;           // 当前指针位置
};

class LogStream : noncopyable {
  typedef LogStream self;   // 类型别名，self 表示 LogStream 类自身
 public:
  typedef FixedBuffer<kSmallBuffer> Buffer;   // 类型别名，Buffer 表示 LogStream 使用的固定大小缓冲区

  // 重载<<运算符，实现通过<<运算符向Buffer_写数据
  self& operator<<(bool);
  self& operator<<(short);
  self& operator<<(unsigned short);
  self& operator<<(int);
  self& operator<<(unsigned int);
  self& operator<<(long);
  self& operator<<(unsigned long);
  self& operator<<(long long);
  self& operator<<(unsigned long long);
  self& operator<<(const void *);
  self& operator<<(float);
  self& operator<<(double);
  self& operator<<(char);
  self& operator<<(const char *);
  self& operator<<(const unsigned char *);
  self& operator<<(const std::string&);
  self& operator<<(const pcrecpp::StringPiece&);
  self& operator<<(const Buffer&);

  // 在 buffer_ 中追加指定长度的字符数据
  void append(const char *data, int len) {
    buffer_.append(data, len);
  }

  // 返回 LogStream 使用的缓冲区
  const Buffer& buffer() const{
    return buffer_;
  }

  // 重置缓冲区
  void resetBuffer() {
    buffer_.reset();
  }

 private:
  template<typename T>
  void formatInteger(T);

  Buffer buffer_;

  static const int kMaxNumericSize = 48;
};

class Fmt {
 public:
  // 构造函数，接受格式字符串和值，用于格式化输出
  template <typename T>
  Fmt(const char *fmt, T val);

  // 返回格式化后的字符串的首地址
  const char *data() const {
    return buf_;
  }

  // 返回格式化后的字符串的长度
  int length() const {
    return length_;
  }

 private:
  char buf_[32];
  int length_;
};

// 重载<<运算符，将 Fmt 对象中的格式化字符串追加到 LogStream 缓冲区中
inline LogStream &operator<<(LogStream& s, const Fmt& fmt) {
  s.append(fmt.data(), fmt.length());
  return s;
}

} // namespace cServer

#endif  // CSERVER_LOG_INCLUDE_LOGSTREAM_