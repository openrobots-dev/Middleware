#pragma once

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
  void initialize();

  void signal_unsafe(unsigned event_index);

  void signal(unsigned event_index);
  Mask wait(const Time &timeout);

public:
  SpinEvent_(bool initialize = true);
};


inline
void SpinEvent_::initialize() {

  R2P_ASSERT(threadp == NULL);

  threadp = chThdSelf();
}


inline
void SpinEvent_::signal_unsafe(unsigned event_index) {

  R2P_ASSERT(event_index < 8 * sizeof(Mask));
  chEvtSignalI(threadp, 1 << event_index);
}


inline
void SpinEvent_::signal(unsigned event_index) {

  R2P_ASSERT(event_index < 8 * sizeof(Mask));
  chEvtSignal(threadp, 1 << event_index);
}


inline
SpinEvent_::Mask SpinEvent_::wait(const Time &timeout) {

  systime_t ticks;
  if      (timeout == Time::IMMEDIATE)  ticks = TIME_IMMEDIATE;
  else if (timeout == Time::INFINITE)   ticks = TIME_INFINITE;
  else                                  ticks = US2ST(timeout.to_us_raw());
  return chEvtWaitAnyTimeout(ALL_EVENTS, ticks);
}


inline
SpinEvent_::SpinEvent_(bool initialize)
:
  threadp(NULL)
{
  if (initialize) {
    this->initialize();
  }
}


} // namespace r2p
