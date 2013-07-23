
#ifndef __R2P__BASEMESSAGE_HPP__
#define __R2P__BASEMESSAGE_HPP__

#include <r2p/common.hpp>

namespace r2p {


#if !defined(R2P_BASEMESSAGE_REFCOUNT_TYPE) || defined(__DOXYGEN__)
#define R2P_BASEMESSAGE_REFCOUNT_TYPE   uint8_t
#endif

#if !defined(R2P_BASEMESSAGE_LENGTH_TYPE) || defined(__DOXYGEN__)
#define R2P_BASEMESSAGE_LENGTH_TYPE     uint8_t
#endif


class BaseMessage {
public:
  typedef R2P_BASEMESSAGE_REFCOUNT_TYPE RefcountType;
  typedef R2P_BASEMESSAGE_LENGTH_TYPE LengthType;

private:
  RefcountType refcount;

public:
  void acquire();
  bool release();
  void reset();

  const uint8_t *get_raw_data() const;

public:
  static void copy(BaseMessage &to, const BaseMessage &from, size_t msg_size);
} R2P_PACKED;


inline
const uint8_t *BaseMessage::get_raw_data() const {

  return reinterpret_cast<const uint8_t *>(&refcount + 1);
}


} // namespace r2p
#endif // __R2P__BASEMESSAGE_HPP__
