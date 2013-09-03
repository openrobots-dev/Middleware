#pragma once

#include <cstddef>
#include <cstring>

#include <r2p/Uncopyable.hpp>
#include <r2p/Utils.hpp>

#include <r2p/SysLock.hpp>
#include "r2p_lld_assert.hpp"

namespace r2p {


#define R2P_PACKED  __attribute__((packed))


template<typename Test, typename Base> inline
void static_cast_check () {

  register Test *pd = 0; (void)pd;
  register Base *pb = pd; (void)pb;
};


} // namespace r2p
