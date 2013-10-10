#pragma once

#include <r2p/common.hpp>
#include <r2p/Time.hpp>
#include <r2p/Thread.hpp>
#include <ch.h>

namespace r2p {


class SpinEvent_ {
public:
  typedef ::eventmask_t Mask;

private:
  typedef ::Thread ChThread;

private:
  Thread *threadp;

public:
  Thread *get_thread() const;
  void set_thread(Thread *threadp);

  void signal_unsafe(unsigned event_index);

  void signal(unsigned event_index);
  Mask wait(const Time &timeout);

public:
  SpinEvent_(Thread *threadp = &Thread::self());
};


inline
Thread *SpinEvent_::get_thread() const {

  return threadp;
}


inline
void SpinEvent_::set_thread(Thread *threadp) {

  this->threadp = threadp;
}


inline
void SpinEvent_::signal_unsafe(unsigned event_index) {

  if (threadp != NULL) {
    R2P_ASSERT(event_index < 8 * sizeof(Mask));
    chEvtSignalI(reinterpret_cast<ChThread *>(threadp), 1 << event_index);
  }
}


inline
void SpinEvent_::signal(unsigned event_index) {

  chSysLock();
  signal_unsafe(event_index);
  chSysUnlock();
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
SpinEvent_::SpinEvent_(Thread *threadp)
:
  threadp(threadp)
{}


} // namespace r2p
