
#ifndef __R2P__SYSLOCK_HPP__
#define __R2P__SYSLOCK_HPP__

#include <r2p/impl/SysLock_.hpp>

namespace r2p {


class SysLock : private Uncopyable {
private:
  SysLock();

public:
  static void acquire();
  static void release();
};


inline
void SysLock::acquire() {

  SysLock_::acquire();
}


inline
void SysLock::release() {

  SysLock_::release();
}


} // namespace r2p
#endif  // __R2P__SYSLOCK_HPP__
