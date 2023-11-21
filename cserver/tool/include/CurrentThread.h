#ifndef CSERVER_TOOL_INCLUDE_CURRENTTHREAD_
#define CSERVER_TOOL_INCLUDE_CURRENTTHREAD_

namespace cServer {
namespace CurrentThread {

extern __thread int t_cachedTid;        // 缓存的线程ID
extern __thread char t_tidString[32];   // 缓存的线程ID的字符串表示
extern __thread int t_tidLen;           // 缓存的线程ID的字符串长度

// 缓存线程ID的函数
void cacheTid();

// 获取当前线程ID的函数，如果尚未缓存，则调用cacheTid进行缓存
inline int tid() {
  if (t_cachedTid == 0) {
    cacheTid();
  }
  // 返回缓存的线程ID
  return t_cachedTid;
}

// 获取缓存的线程ID的字符串表示的函数
inline const char* tidString() {
  return t_tidString;
}

}  // namespace CurrentThread
}  // namespace cServer

#endif  // CSERVER_TOOL_INCLUDE_CURRENTTHREAD_
