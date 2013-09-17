#pragma once

#if !defined(R2P_ASSERT) || defined(__DOXYGEN__)
#define R2P_ASSERT(expr)    { chDbgCheck(expr, R2P_ASSERT); }
#endif
