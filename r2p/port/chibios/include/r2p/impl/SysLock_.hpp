
#ifndef __R2P__SYSLOCK__HPP__
#define __R2P__SYSLOCK__HPP__

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
#endif  // __R2P__SYSLOCK__HPP__
