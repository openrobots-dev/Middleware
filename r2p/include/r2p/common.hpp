#pragma once

#include <cstddef>
#include <cstring>

#include <r2p/Uncopyable.hpp>
#include <r2p/Utils.hpp>

#include <r2p/SysLock.hpp>
#include "r2p_lld_assert.hpp" // TODO: add as user port

namespace r2p {


#ifndef R2P_MEMORY_ALIGNED
#define R2P_MEMORY_ALIGNED  __attribute__((aligned(sizeof(unsigned))))
#endif

#define R2P_PACKED          __attribute__((packed))
#define R2P_FORCE_INLINE    inline __attribute__((always_inline))

#define R2P_APP_CONFIG      __attribute__((section("app_config")))


template<typename Test, typename Base> R2P_FORCE_INLINE
void static_cast_check () {

  register const Test *const pd = NULL;
  register const Base *const pb = pd;
  (void)pd;
  (void)pb;
};


inline
size_t bit_mask(size_t num_bits) {

  return (static_cast<size_t>(1) << num_bits) - 1;
}


inline
size_t byte_mask(size_t num_bytes) {

  return bit_mask(num_bytes << 3);
}


template<typename T> inline
size_t type_mask() {

  return byte_mask(sizeof(T));
}


template<typename T> inline
size_t compute_chunk_size(const T *startp, const T *endxp) {

  return static_cast<size_t>(
    reinterpret_cast<ptrdiff_t>(endxp) - reinterpret_cast<ptrdiff_t>(startp)
  );
}


inline
bool check_bounds(const void *valuep, size_t valuelen,
                  const void *slotp, size_t slotlen) {

  register const uint8_t *const vp = reinterpret_cast<const uint8_t *>(valuep);
  register const uint8_t *const sp = reinterpret_cast<const uint8_t *>(slotp);

  return vp >= sp && (vp + valuelen) <= (sp + slotlen);
}


} // namespace r2p
