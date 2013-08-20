
#include <r2p/Message.hpp>
#include <cstring>

namespace r2p {


void Message::acquire() {

  SysLock::acquire();
  acquire_unsafe();
  SysLock::release();
}


bool Message::release() {

  return safeguard(release_unsafe());
}


void Message::reset() {

  SysLock::acquire();
  reset_unsafe();
  SysLock::release();
}


void Message::copy(Message &to, const Message &from, size_t msg_size) {

  R2P_ASSERT(msg_size >= sizeof(RefcountType));

  ::memcpy(&to.refcount + 1, &from.refcount + 1,
           msg_size - sizeof(RefcountType));
}


}; // namespace r2p
