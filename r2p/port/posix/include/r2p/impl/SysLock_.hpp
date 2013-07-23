
#ifndef __R2P__SYSLOCK__HPP__
#define __R2P__SYSLOCK__HPP__

namespace r2p {


class SysLock_ : private Uncopyable {
private:
  SysLock_();

public:
  static void acquire();
  static void release();

private:
  static volatile unsigned exclusion;
};


inline
void SysLock_::acquire() {

  while (__sync_lock_test_and_set(&exclusion, 1)) {
    while (exclusion) {}
  }
}


inline
void SysLock_::release() {

  __sync_lock_release(&exclusion);
}


} // namespace r2p
#endif  // __R2P__SYSLOCK__HPP__
