#pragma once

#include <r2p/common.hpp>
#include <r2p/impl/Semaphore_.hpp>

namespace r2p {


class Semaphore : private Uncopyable {
public:
  typedef Semaphore_::Count Count;

private:
  Semaphore_ impl;

public:
  void reset_unsafe(Count value = 0);
  void signal_unsafe();
  void wait_unsafe();
  bool wait_unsafe(const Time &timeout);

  void reset(Count value = 0);
  void signal();
  void wait();
  bool wait(const Time &timeout);

public:
  Semaphore(Count value = 0);
};


inline
void Semaphore::reset_unsafe(Count value) {

  impl.reset_unsafe(value);
}


inline
void Semaphore::signal_unsafe() {

  impl.signal_unsafe();
}


inline
void Semaphore::wait_unsafe() {

  impl.wait_unsafe();
}


inline
bool Semaphore::wait_unsafe(const Time &timeout) {

  return impl.wait_unsafe(timeout);
}


inline
void Semaphore::reset(Count value) {

  impl.reset(value);
}


inline
void Semaphore::signal() {

  impl.signal();
}


inline
void Semaphore::wait() {

  impl.wait();
}


inline
bool Semaphore::wait(const Time &timeout) {

  return impl.wait(timeout);
}


inline
Semaphore::Semaphore(Count value)
:
  impl(value)
{}


} // namespace r2p
