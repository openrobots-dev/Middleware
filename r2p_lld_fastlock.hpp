
#ifndef __R2P_LLD_SYSLOCK_HPP__
#define __R2P_LLD_SYSLOCK_HPP__

#include <ch.h>

namespace r2p {


class FastLock {
public:
  static void init();
  static void acquire();
  static void release();
};


inline
void FastLock::acquire() {
  chSysLock();
}


inline
void FastLock::release() {
  chSysUnlock();
}


inline
void FastLock::init() {}


} // namespace r2p
#endif  // __R2P_LLD_SYSLOCK_HPP__
