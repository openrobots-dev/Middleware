#pragma once

#include <r2p/common.hpp>

namespace r2p {


#if !defined(R2P_MESSAGE_REFCOUNT_TYPE) || defined(__DOXYGEN__)
#define R2P_MESSAGE_REFCOUNT_TYPE   uint16_t
#endif

#if !defined(R2P_MESSAGE_LENGTH_TYPE) || defined(__DOXYGEN__)
#define R2P_MESSAGE_LENGTH_TYPE     uint8_t
#endif

class Transport;


// TODO: Add refcount as a decorator, user should only declare the contents type
class Message {
public:
  typedef R2P_MESSAGE_REFCOUNT_TYPE RefcountType;
  typedef R2P_MESSAGE_LENGTH_TYPE LengthType;

private:
#if R2P_USE_BRIDGE_MODE
  Transport *sourcep R2P_PACKED;
#endif
  RefcountType refcount R2P_PACKED;

public:
  const uint8_t *get_raw_data() const;
#if R2P_USE_BRIDGE_MODE
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
  static const Message &get_msg_from_raw_data(const uint8_t *datap);
  static size_t get_payload_size(size_t type_size);
  static size_t get_type_size(size_t payload_size);

  template<typename MessageType>
  static void copy(MessageType &to, const MessageType &from);

  template<typename MessageType>
  static size_t get_payload_size();

  template<typename MessageType>
  static void reset_payload(MessageType &msg);
} R2P_PACKED;


inline
const uint8_t *Message::get_raw_data() const {

  return reinterpret_cast<const uint8_t *>(&refcount + 1);
}

// TODO: menate di stile
inline
const Message &Message::get_msg_from_raw_data(const uint8_t * datap) {
#if R2P_USE_BRIDGE_MODE
  return *reinterpret_cast<const Message *>(
    datap - (sizeof(Transport *) + sizeof(RefcountType))
  );
#else
  return *reinterpret_cast<const Message *>(datap - sizeof(RefcountType));
#endif
}


#if R2P_USE_BRIDGE_MODE

inline
Transport *Message::get_source() const {

  return sourcep;
}


inline
void Message::set_source(Transport *sourcep) {

  this->sourcep = sourcep;
}

#endif // R2P_USE_BRIDGE_MODE


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

#if R2P_USE_BRIDGE_MODE
  sourcep = NULL;
#endif
  refcount = 0;
}


inline
Message::Message()
:
#if R2P_USE_BRIDGE_MODE
  sourcep(NULL),
#endif
  refcount(0)
{}


inline
size_t Message::get_payload_size(size_t type_size) {

#if R2P_USE_BRIDGE_MODE
  return type_size - (sizeof(Transport *) + sizeof(RefcountType));
#else
  return type_size - sizeof(RefcountType);
#endif
}


inline
size_t Message::get_type_size(size_t payload_size) {

#if R2P_USE_BRIDGE_MODE
  return payload_size + (sizeof(Transport *) + sizeof(RefcountType));
#else
  return payload_size + sizeof(RefcountType);
#endif
}


template<typename MessageType> inline
void Message::copy(MessageType &to, const MessageType &from) {

  static_cast_check<MessageType, Message>();
  return copy(to, from, sizeof(MessageType));
}


template<typename MessageType> inline
size_t Message::get_payload_size() {

  static_cast_check<MessageType, Message>();
  return get_payload_size(sizeof(MessageType));
}


template<typename MessageType> inline
void Message::reset_payload(MessageType &msg) {

  static_cast_check<MessageType, Message>();
  memset(&msg.refcount + 1, 0, get_payload_size<MessageType>());
}


} // namespace r2p
