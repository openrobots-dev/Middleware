#pragma once

#include <cstddef>
#include <cstring>

#include <r2p/Uncopyable.hpp>
#include <r2p/Utils.hpp>

#include <r2p/SysLock.hpp>
#include "r2p_lld_assert.hpp"

namespace r2p {


#define R2P_PACKED  __attribute__((packed))


template<typename ParentType, typename ChildType>
__attribute__((always_inline))
void static_cast_check(const ChildType &child) {

  // Should compile without errors
  (void)static_cast<ParentType>(child);
}


} // namespace r2p
