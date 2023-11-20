#include "LogStream.h"
#include <algorithm>
#include <limits>
#include <assert.h>

namespace cServer {

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
const char digitsHex[] = "0123456789ABCDEF";

// 将整数转为字符数组
template <typename T>
size_t convert(char buf[], T value) {
  T i = value;
  char *p = buf;

  do {
    int lsd = static_cast<int>(i % 10);
    i /= 10;
    *p++ = zero[lsd];
  } while (i != 0);

  if (value < 0) {
    *p++ = '-';
  }
  *p = '\0';
  std::reverse(buf, p);     // 反转缓冲区中的字符串，使其成为正常的顺序

  return p - buf;   // 返回转换后的字符串的长度
}

// 将16进制数转为字符数组
size_t converHex(char buf[], uintptr_t value) {
  uintptr_t i = value;  // 无符号整数
  char *p = buf;

  do {
    int lsd = static_cast<int>(i % 16);
    i /= 16;
    *p++ = digitsHex[lsd];
  } while (i != 0);
  *p = '\0';
  std::reverse(buf, p);

  return p - buf;   // 返回转换后的十六进制字符串的长度
}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

template <int SIZE>
const char *FixedBuffer<SIZE>::debugString() {
  *cur_ = '\0';
  return data_;
}

template <typename T>
void LogStream::formatInteger(T v) {
  if (buffer_.avail() >= kMaxNumericSize) {
    size_t len = convert(buffer_.current(), v);
    buffer_.add(len);
  }
}

LogStream &LogStream::operator<<(bool b) {
  buffer_.append(b ? "1" : "0", 1);
  return *this;
}

LogStream &LogStream::operator<<(short v) {
  *this << static_cast<int>(v);
  return *this;
}

LogStream &LogStream::operator<<(unsigned short v) {
  *this << static_cast<unsigned int>(v);
  return *this;
}

LogStream &LogStream::operator<<(int v) {
  formatInteger(v);
  return *this;
}

LogStream &LogStream::operator<<(unsigned int v) {
  formatInteger(v);
  return *this;
}

LogStream &LogStream::operator<<(long v) {
  formatInteger(v);
  return *this;
}

LogStream &LogStream::operator<<(unsigned long v) {
  formatInteger(v);
  return *this;
}

LogStream &LogStream::operator<<(long long v) {
  formatInteger(v);
  return *this;
}

LogStream &LogStream::operator<<(unsigned long long v) {
  formatInteger(v);
  return *this;
}

LogStream &LogStream::operator<<(const void* p) 
{
  auto v = reinterpret_cast<uintptr_t>(p);    // 将指针 p 转换为无符号整数
  if(buffer_.avail() >= kMaxNumericSize){     // 检查缓冲区是否有足够空间
    char* buf = buffer_.current();
    buf[0] = '0';
    buf[1] = 'x';
    size_t len = converHex(buf + 2, v);       // 调用 converHex 函数将 uintptr_t 转换为十六进制字符串
    buffer_.add(len + 2);                     // 移动当前指针位置，更新缓冲区已使用的长度
  }

  return *this;
}

LogStream &LogStream::operator<<(float b) {
  *this <<static_cast<double>(b);
  return *this;
}

LogStream &LogStream::operator<<(double v) {
  if(buffer_.avail() >= kMaxNumericSize){
    int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
    buffer_.add(len);
  }
  return *this;
}

LogStream &LogStream::operator<<(char b) {
  buffer_.append(&b, 1);
  return *this;
}

LogStream &LogStream::operator<<(const char *b) 
{
  if(b){
    buffer_.append(b, strlen(b));
  } else {
    buffer_.append("(null)", 6);
  }

  return *this;
}

LogStream &LogStream::operator<<(const unsigned char *str) {
  return operator<<(reinterpret_cast<const char *>(str));
}

LogStream &LogStream::operator<<(const std::string &b) 
{
  buffer_.append(b.c_str(), b.size());
  return *this;
}

LogStream &LogStream::operator<<(const pcrecpp::StringPiece &v)
{
  buffer_.append(v.data(), v.size());
  return *this;
}

LogStream &LogStream::operator<<(const Buffer &v)
{
  *this << v.toStringPiece();
  return *this;
}

template<typename T>
Fmt::Fmt(const char *fmt, T val) {
  length_ = snprintf(buf_, sizeof(buf_), fmt, val);
  assert(static_cast<size_t>(length_) < sizeof(buf_));
}

} // namespace cServer