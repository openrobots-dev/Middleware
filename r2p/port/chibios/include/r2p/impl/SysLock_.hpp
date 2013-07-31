#pragma once

#include <r2p/common.hpp>
#include <ch.h>

namespace r2p {


class SysLock_ : private Uncopyable {
private:
  SysLock_();

public:
  static void acquire();
  static void release();
};


inline
void SysLock_::acquire() {

  chSysLock();
}


inline
void SysLock_::release() {

  chSysUnlock();
}


} // namespace r2p
