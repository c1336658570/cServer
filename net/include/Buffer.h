#ifndef CSERVER_NET_INCLUDE_BUFFER_
#define CSERVER_NET_INCLUDE_BUFFER_


#include <algorithm>
#include <string>
#include <vector>

#include <assert.h>
//#include <unistd.h>  // ssize_t

namespace cServer {

class Buffer {
 public:
  static const size_t kCheapPrepend = 8;      // 在buffer_前面预留的空间大小
  static const size_t kInitialSize = 1024;    // 初始分配的buffer_大小

  Buffer()
      : buffer_(kCheapPrepend + kInitialSize),    // 构造函数初始化buffer_，包括预留空间
        readerIndex_(kCheapPrepend),              // 读取位置的初始索引
        writerIndex_(kCheapPrepend) {             // 写入位置的初始索引
    assert(readableBytes() == 0);                 // 确保初始时可读字节数为0
    assert(writableBytes() == kInitialSize);      // 确保初始时可写字节数为kInitialSize
    assert(prependableBytes() == kCheapPrepend);  // 确保初始时预留空间大小为kCheapPrepend
  }

  // 默认的拷贝构造函数、析构函数和赋值运算符

  // 交换两个Buffer对象的内容
  void swap(Buffer &rhs) {
    buffer_.swap(rhs.buffer_);                      // 交换buffer_的内容
    std::swap(readerIndex_, rhs.readerIndex_);      // 交换读取位置的索引
    std::swap(writerIndex_, rhs.writerIndex_);      // 交换写入位置的索引
  }

  // 返回可读字节数
  size_t readableBytes() const {
    return writerIndex_ - readerIndex_;
  }

  // 返回可写字节数
  size_t writableBytes() const {
    return buffer_.size() - writerIndex_;
  }

  // 返回预留空间的字节数
  size_t prependableBytes() const {
    return readerIndex_;
  }

  // 返回指向可读数据的指针
  const char *peek() const {
    return begin() + readerIndex_;
  }

  // retrieve函数返回void，以避免类似于 string str(retrieve(readableBytes()), readableBytes()); 这样的表达式，其中两个函数的执行顺序是不确定的。
  // 从缓冲区中读取指定长度的数据，更新读取位置的索引。
  void retrieve(size_t len) {
    assert(len <= readableBytes());     // 确保指定的长度不超过可读字节数
    readerIndex_ += len;                // 更新读取位置的索引，将其向后移动指定的长度
  }

  // 从Buffer中读取数据直到指定位置
  void retrieveUntil(const char *end) {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }

  // 重置Buffer，将读取和写入位置的索引设置为初始状态
  void retrieveAll() {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }

  // 从Buffer中读取所有数据并返回一个字符串
  std::string retrieveAsString() {
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
  }

  // 将字符串追加到Buffer中
  void append(const std::string &str) {
    append(str.data(), str.length());
  }

  // 将指定长度的数据追加到Buffer中
  void append(const char *data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
  }

  // 将指定长度的数据追加到Buffer中
  void append(const void *data, size_t len) {
    append(static_cast<const char *>(data), len);
  }

  // 确保Buffer中有足够的可写字节数
  void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);
    }
    assert(writableBytes() >= len);
  }

  // 返回写入位置的指针
  char *beginWrite() {
    return begin() + writerIndex_;
  }

  // 返回写入位置的指针（const版本）
  const char *beginWrite() const {
    return begin() + writerIndex_;
  }

  // 更新写入位置的索引
  void hasWritten(size_t len) {
    writerIndex_ += len;
  }
  
  // 在Buffer的前面添加指定长度的数据
  void prepend(const void *data, size_t len) {
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char *d = static_cast<const char *>(data);
    std::copy(d, d + len, begin() + readerIndex_);
  }

  // 缩小Buffer的大小，释放不需要的空间
  void shrink(size_t reserve) {
    std::vector<char> buf(kCheapPrepend + readableBytes() + reserve);
    std::copy(peek(), peek() + readableBytes(), buf.begin() + kCheapPrepend);
    buf.swap(buffer_);
  }

  // 从文件描述符中读取数据到Buffer中
  ssize_t readFd(int fd, int *savedErrno);

 private:
  // 返回Buffer的起始地址
  char *begin() {
    return &*buffer_.begin();
  }

  // 返回Buffer的起始地址（const版本）
  const char *begin() const {
    return &*buffer_.begin();
  }

  // 确保Buffer中有足够的空间来容纳指定长度的数据
  void makeSpace(size_t len) {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
      // 缓冲区可写字节数+预留字节数如果比要写入的长度len和kCheapPrepend之和小的话就要扩容
      buffer_.resize(writerIndex_ + len);
    } else {
      // 将可读数据移动到Buffer的前面，为新数据腾出空间
      assert(kCheapPrepend < readerIndex_);
      size_t readable = readableBytes();
      std::copy(begin() + readerIndex_, begin() + writerIndex_,
                begin() + kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
      assert(readable == readableBytes());
    }
  }

 private:
  std::vector<char> buffer_;      // 存储数据的缓冲区
  size_t readerIndex_;            // 读取位置的索引
  size_t writerIndex_;            // 写入位置的索引
};

}  // namespace cServer

#endif  // CSERVER_NET_INCLUDE_BUFFER_
