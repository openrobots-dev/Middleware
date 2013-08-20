#pragma once

#include <cstddef>
#include <cstring>

#include <r2p/Uncopyable.hpp>
#include <r2p/Utils.hpp>

#include <r2p/SysLock.hpp>
#include "r2p_lld_assert.hpp"

namespace r2p {


#define R2P_PACKED  __attribute__((packed))


template<typename Type> __attribute__((always_inline))
Type safeguard(Type value) {

  SysLock::acquire();
  Type safe = value;
  SysLock::release();
  return safe;
}


} // namespace r2p
