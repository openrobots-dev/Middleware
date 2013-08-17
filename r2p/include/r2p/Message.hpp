#pragma once

#include <r2p/common.hpp>

namespace r2p {


#if !defined(R2P_BASEMESSAGE_REFCOUNT_TYPE) || defined(__DOXYGEN__)
#define R2P_BASEMESSAGE_REFCOUNT_TYPE   uint8_t
#endif

#if !defined(R2P_BASEMESSAGE_LENGTH_TYPE) || defined(__DOXYGEN__)
#define R2P_BASEMESSAGE_LENGTH_TYPE     uint8_t
#endif


// TODO: Add refcount as a decorator, user should only declare the contents type
class Message {
public:
  typedef R2P_BASEMESSAGE_REFCOUNT_TYPE RefcountType;
  typedef R2P_BASEMESSAGE_LENGTH_TYPE LengthType;

private:
  RefcountType refcount;

public:
  void acquire_unsafe();
  bool release_unsafe();
  void reset_unsafe();

  void acquire();
  bool release();
  void reset();

  const uint8_t *get_raw_data() const;

protected:
  Message();

public:
  static void copy(Message &to, const Message &from, size_t msg_size);
} R2P_PACKED;


inline
const uint8_t *Message::get_raw_data() const {

  return reinterpret_cast<const uint8_t *>(&refcount + 1); // FIXME: does not work without packed attribute (and maybe without gcc -O3)
}


inline
void Message::acquire_unsafe() {

  R2P_ASSERT(refcount < ((1 << (8 * sizeof(refcount) - 1)) - 1));
  ++refcount;
}


inline
bool Message::release_unsafe() {

  R2P_ASSERT(refcount > 0);
  return --refcount > 0;
}


inline
void Message::reset_unsafe() {

  refcount = 0;
}


inline
Message::Message()
:
  refcount(0)
{}


} // namespace r2p
