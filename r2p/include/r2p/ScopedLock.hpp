#pragma once

#include <r2p/common.hpp>

namespace r2p {


template<typename Lockable>
class ScopedLock : private Uncopyable {
private:
  Lockable &lock;

public:
  ScopedLock(Lockable &lock);
  ~ScopedLock();
};


template<typename Lockable> inline
ScopedLock<Lockable>::ScopedLock(Lockable &lock)
:
  lock(lock)
{
  this->lock.acquire();
}


template<typename Lockable> inline
ScopedLock<Lockable>::~ScopedLock() {

  this->lock.release();
}


} // namespace r2p
