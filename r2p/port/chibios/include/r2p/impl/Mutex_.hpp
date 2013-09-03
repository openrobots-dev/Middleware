#pragma once

#include <r2p/common.hpp>
#include <ch.h>

namespace r2p {


class Mutex_ : private Uncopyable {
private:
  ::Mutex impl;

public:
  void initialize();

  void acquire_unsafe();
  void release_unsafe();

  void acquire();
  void release();

  ::Mutex &get_impl();

public:
  Mutex_();
  Mutex_(bool initialize);
};


inline
void Mutex_::initialize() {

  chMtxInit(&impl);
}


inline
void Mutex_::acquire_unsafe() {

  chMtxLockS(&impl);
}


inline
void Mutex_::release_unsafe() {

  chMtxUnlockS();
}


inline
void Mutex_::acquire() {

  chMtxLock(&impl);
}


inline
void Mutex_::release() {

  chMtxUnlock();
}


inline
::Mutex &Mutex_::get_impl() {

  return impl;
}


inline
Mutex_::Mutex_() {

  this->initialize();
}


inline
Mutex_::Mutex_(bool initialize) {

  if (initialize) {
    this->initialize();
  }
}


} // namespace r2p
