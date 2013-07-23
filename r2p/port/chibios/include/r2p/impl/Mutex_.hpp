
#ifndef __R2P__MUTEX__HPP__
#define __R2P__MUTEX__HPP__

#include <r2p/common.hpp>
#include <ch.h>

namespace r2p {


class Mutex_ : private Uncopyable {
private:
  ::Mutex impl;

public:
  void acquire_unsafe();
  void release_unsafe();

  void acquire();
  void release();

public:
  Mutex_();
};


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
Mutex_::Mutex_() {

  chMtxInit(&impl);
}


} // namespace r2p
#endif // __R2P__MUTEX__HPP__
