#pragma once

#include <r2p/common.hpp>
#include <r2p/impl/SpinEvent_.hpp>
#include <r2p/Time.hpp>
#include <r2p/Thread.hpp>

namespace r2p {


class SpinEvent : private Uncopyable {
public:
  typedef SpinEvent_::Mask Mask;

  enum { MAX_INDEX = (sizeof(Mask) * 8) - 1 };

private:
  SpinEvent_ impl;

public:
  Thread *get_thread() const;
  void set_thread(Thread *threadp);

  void signal_unsafe(unsigned event_index);

  void signal(unsigned event_index);
  Mask wait(const Time &timeout);

public:
  SpinEvent(Thread *threadp = &Thread::self());
};


inline
Thread *SpinEvent::get_thread() const {

  return impl.get_thread();
}


inline
void SpinEvent::set_thread(Thread *threadp) {

  impl.set_thread(threadp);
}


inline
void SpinEvent::signal_unsafe(unsigned event_index) {

  impl.signal_unsafe(event_index);
}


inline
void SpinEvent::signal(unsigned event_index) {

  impl.signal(event_index);
}


inline
SpinEvent::Mask SpinEvent::wait(const Time &timeout) {

  return impl.wait(timeout);
}


inline
SpinEvent::SpinEvent(Thread *threadp)
:
  impl(threadp)
{}


} // namespace r2p
