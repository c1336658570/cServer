#include "Timer.h"

namespace cServer {

void Timer::restart(Timestamp now) {
  if (repeat_) {
    // 如果定时器是重复定时器，则更新到期时间为当前时间加上重复间隔
    expiration_ = addTime(now, interval_);
  } else {
    // 如果定时器不是重复定时器，则将到期时间设置为无效时间戳，表示不再重复触发
    expiration_ = Timestamp::invalid();
  }
}

}
