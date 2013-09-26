#pragma once

#include <r2p/common.hpp>

namespace r2p {


#if !defined(R2P_MESSAGE_REFCOUNT_TYPE) || defined(__DOXYGEN__)
#define R2P_MESSAGE_REFCOUNT_TYPE   uint8_t
#endif

#if !defined(R2P_MESSAGE_LENGTH_TYPE) || defined(__DOXYGEN__)
#define R2P_MESSAGE_LENGTH_TYPE     uint8_t
#endif

#if !defined(R2P_MESSAGE_TRACKS_SOURCE) || defined(__DOXYGEN__)
#define R2P_MESSAGE_TRACKS_SOURCE   1
#endif


class Transport;


// TODO: Add refcount as a decorator, user should only declare the contents type
class Message {
public:
  typedef R2P_MESSAGE_REFCOUNT_TYPE RefcountType;
  typedef R2P_MESSAGE_LENGTH_TYPE LengthType;

private:
#if R2P_MESSAGE_TRACKS_SOURCE
  Transport *sourcep;
#endif
  RefcountType refcount;

public:
  const uint8_t *get_raw_data() const;
#if R2P_MESSAGE_TRACKS_SOURCE
  Transport *get_source() const;
  void set_source(Transport *sourcep);
#endif

  void acquire_unsafe();
  bool release_unsafe();
  void reset_unsafe();

  void acquire();
  bool release();
  void reset();

protected:
  Message();

public:
  static void copy(Message &to, const Message &from, size_t msg_size);

  template<typename MessageType>
  static void reset_payload(MessageType &msg);
} R2P_PACKED;


inline
const uint8_t *Message::get_raw_data() const {

  return reinterpret_cast<const uint8_t *>(&refcount + 1);
}


#if R2P_MESSAGE_TRACKS_SOURCE

inline
Transport *Message::get_source() const {

  return sourcep;
}


inline
void Message::set_source(Transport *sourcep) {

  this->sourcep = sourcep;
}

#endif // R2P_MESSAGE_TRACKS_SOURCE


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

#if R2P_MESSAGE_TRACKS_SOURCE
  sourcep = NULL;
#endif
  refcount = 0;
}


inline
Message::Message()
:
#if R2P_MESSAGE_TRACKS_SOURCE
  sourcep(NULL),
#endif
  refcount(0)
{}


template<typename MessageType> inline
void Message::reset_payload(MessageType &msg) {

  static_cast_check<MessageType, Message>();

#if R2P_MESSAGE_TRACKS_SOURCE
  memset(&msg.refcount + 1, 0,
         sizeof(MessageType) - sizeof(RefcountType) - sizeof(Transport *));
#else
  memset(&msg.refcount + 1, 0, sizeof(MessageType) - sizeof(RefcountType));
#endif
}


} // namespace r2p
