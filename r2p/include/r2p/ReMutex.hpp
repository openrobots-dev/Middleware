#pragma once

#include <r2p/common.hpp>
#include <r2p/Mutex.hpp>

namespace r2p {


class ReMutex : private Uncopyable {
private:
  size_t counter;
  Mutex mutex;

public:
  void initialize();

  void acquire_unsafe();
  void release_unsafe();

  void acquire();
  void release();

public:
  ReMutex();
  explicit ReMutex(bool initialize);
};


inline
void ReMutex::initialize() {

  counter = 0;
  mutex.initialize();
}


inline
void ReMutex::acquire_unsafe() {

  if (counter++ == 0) {
    mutex.acquire_unsafe();
  }
}


inline
void ReMutex::release_unsafe() {

  R2P_ASSERT(counter > 0);

  if (--counter == 0) {
    mutex.release_unsafe();
  }
}


inline
void ReMutex::acquire() {

  SysLock::acquire();
  acquire_unsafe();
  SysLock::release();
}


inline
void ReMutex::release() {

  SysLock::acquire();
  release_unsafe();
  SysLock::release();
}


inline
ReMutex::ReMutex()
:
  counter(0),
  mutex()
{}


inline
ReMutex::ReMutex(bool initialize)
:
  counter(0),
  mutex(initialize)
{}


} // namespace r2p
