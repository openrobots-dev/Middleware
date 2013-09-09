#pragma once

#include <cstddef>
#include <cstring>

#include <r2p/Uncopyable.hpp>
#include <r2p/Utils.hpp>

#include <r2p/SysLock.hpp>
#include "r2p_lld_assert.hpp" // TODO: add as user port

namespace r2p {


#define R2P_PACKED          __attribute__((packed))
#define R2P_FORCE_INLINE    inline __attribute__((always_inline))


template<typename Test, typename Base> R2P_FORCE_INLINE
void static_cast_check () {

  register const Test *const pd = NULL;
  register const Base *const pb = pd;
  (void)pd;
  (void)pb;
};


} // namespace r2p
