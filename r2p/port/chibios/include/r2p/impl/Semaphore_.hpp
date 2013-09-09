#pragma once

#include <r2p/common.hpp>
#include <r2p/Time.hpp>
#include <ch.h>

namespace r2p {


class Semaphore_ : private Uncopyable {
public:
  typedef cnt_t Count;

private:
  ::Semaphore impl;

public:
  void initialize(Count value = 0);

  void reset_unsafe(Count value = 0);
  void signal_unsafe();
  void wait_unsafe();
  bool wait_unsafe(const Time &timeout);

  void reset(Count value = 0);
  void signal();
  void wait();
  bool wait(const Time &timeout);

  ::Semaphore &get_impl();

public:
  Semaphore_(Count value = 0);
  explicit Semaphore_(bool initialize, Count value = 0);
};


inline
void Semaphore_::initialize(Count value) {

  chSemInit(&impl, value);
}


inline
void Semaphore_::reset_unsafe(Count value) {

  chSemResetI(&impl, value);
}


inline
void Semaphore_::signal_unsafe() {

  chSemSignalI(&impl);
}


inline
void Semaphore_::wait_unsafe() {

  chSemWaitS(&impl);
}


inline
bool Semaphore_::wait_unsafe(const Time &timeout) {

  return chSemWaitTimeoutS(&impl, US2ST(timeout.to_us_raw())) == RDY_OK;
}


inline
void Semaphore_::reset(Count value) {

  chSemReset(&impl, value);
}


inline
void Semaphore_::signal() {

  chSemSignal(&impl);
}


inline
void Semaphore_::wait() {

  chSemWait(&impl);
}


inline
bool Semaphore_::wait(const Time &timeout) {

  return chSemWaitTimeout(&impl, US2ST(timeout.to_us_raw())) == RDY_OK;
}


inline
::Semaphore &Semaphore_::get_impl() {

  return impl;
}


inline
Semaphore_::Semaphore_(Count value) {

  initialize(value);
}


inline
Semaphore_::Semaphore_(bool initialize, Count value) {

  if (initialize) {
    this->initialize(value);
  }
}


} // namespace r2p
