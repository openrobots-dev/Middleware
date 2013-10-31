
#include <r2p/Message.hpp>
#include <cstring>

namespace r2p {


void Message::acquire() {

  SysLock::acquire();
  acquire_unsafe();
  SysLock::release();
}


bool Message::release() {

  SysLock::acquire();
  bool floating = release_unsafe();
  SysLock::release();
  return floating;
}


void Message::reset() {

  SysLock::acquire();
  reset_unsafe();
  SysLock::release();
}


void Message::copy(Message &to, const Message &from, size_t type_size) {

  R2P_ASSERT(type_size >= sizeof(RefcountType));

  memcpy( // TODO: just copy the payload with payload_size instead of type_size
    &to.refcount + 1, &from.refcount + 1,
#if R2P_USE_BRIDGE_MODE
    type_size - (sizeof(Transport *) + sizeof(RefcountType))
#else
    type_size - sizeof(RefcountType)
#endif
  );
}


}; // namespace r2p
