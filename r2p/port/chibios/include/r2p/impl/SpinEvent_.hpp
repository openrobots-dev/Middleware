
#ifndef __R2P__SPINEVENT__HPP__
#define __R2P__SPINEVENT__HPP__

#include <r2p/common.hpp>
#include <r2p/Time.hpp>
#include <ch.h>

namespace r2p {


class SpinEvent_ {
public:
  typedef ::eventmask_t Mask;

private:
  ::Thread *threadp;

public:
  bool signal(unsigned event_index) {
    R2P_ASSERT(event_index < 8 * sizeof(Mask));
    chEvtSignal(threadp, 1 << event_index);
    return true;
  }

  Mask wait(const Time &timeout) {
    systime_t ticks;
    if (timeout == Time::IMMEDIATE) ticks = TIME_IMMEDIATE;
    else if (timeout == Time::INFINITE) ticks = TIME_INFINITE;
    else ticks = US2ST(timeout.to_us_raw());
    return chEvtWaitAnyTimeout(ALL_EVENTS, ticks);
  }

public:
  SpinEvent_() : threadp(chThdSelf()) {}
};


} // namespace r2p
#endif // __R2P__SPINEVENT__HPP__
