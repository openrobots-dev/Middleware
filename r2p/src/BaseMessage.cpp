
#include <r2p/BaseMessage.hpp>
#include <cstring>

namespace r2p {


void BaseMessage::acquire() {

  SysLock::acquire();
  R2P_ASSERT(refcount < ((1 << (8 * sizeof(refcount) - 1)) - 1));
  ++refcount;
  SysLock::release();
}


bool BaseMessage::release() {

  SysLock::acquire();
  R2P_ASSERT(refcount > 0);
  bool has_refs = --refcount > 0;
  SysLock::release();
  return has_refs;
}


void BaseMessage::reset() {

  SysLock::acquire();
  refcount = 0;
  SysLock::release();
}


void BaseMessage::copy(BaseMessage &to, const BaseMessage &from,
                       size_t msg_size) {

  R2P_ASSERT(msg_size >= sizeof(RefcountType));

  ::memcpy(&to.refcount + 1, &from.refcount + 1,
           msg_size - sizeof(RefcountType));
}


}; // namespace r2p
