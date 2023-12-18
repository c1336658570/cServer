// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "Buffer.h"
#include "Logging.h"

#include <errno.h>
#include <memory.h>
#include <sys/uio.h>

namespace cServer {

// 从文件描述符（fd）中读取数据到缓冲区中，并返回读取的字节数。如果发生错误，通过savedErrno参数返回错误码。
ssize_t Buffer::readFd(int fd, int *savedErrno) {
  char extrabuf[65536];   // 临时缓冲区，用于存储额外的数据
  struct iovec vec[2];    // 定义两个iovec结构体，用于readv函数读取数据
  const size_t writable = writableBytes();      // 获取当前缓冲区的可写字节数
  vec[0].iov_base = begin() + writerIndex_;     // 设置第一个iovec结构体，指向缓冲区可写位置
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;                   // 设置第二个iovec结构体，指向临时缓冲区
  vec[1].iov_len = sizeof(extrabuf);
  const ssize_t n = readv(fd, vec, 2);          // 使用readv函数从文件描述符中读取数据到iovec结构体数组中
  if (n < 0) {
    // 如果readv函数返回错误，将错误码保存到savedErrno中
    *savedErrno = errno;
  } else if (static_cast<size_t>(n) <= writable) {
    // 如果读取的数据量不超过当前缓冲区的可写字节数，直接更新写入位置
    writerIndex_ += n;
  } else {
    // 如果读取的数据量超过当前缓冲区的可写字节数，需要将多余的数据追加到缓冲区末尾
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;     // 返回读取的总字节数
}

}  // namespace cServer
