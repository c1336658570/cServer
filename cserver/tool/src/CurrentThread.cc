#include "CurrentThread.h"

#include <sys/syscall.h>
#include <unistd.h>

#include <cassert>
#include <string>

namespace cServer {
namespace CurrentThread {

__thread int t_cachedTid = 0;   // 缓存的线程ID
__thread char t_tidString[32];  // 缓存的线程ID的字符串表示
__thread int t_tidLen = 0;      // 缓存的线程ID的字符串长度

// 缓存线程ID的函数
void cacheTid() {
  if (t_cachedTid == 0) {
    // 如果线程ID尚未缓存，则获取当前线程ID并缓存
    t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
    // 将线程ID格式化为字符串，并计算字符串长度
    t_tidLen = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
  }
}

}  // namespace CurrentThread
}  // namespace cServer