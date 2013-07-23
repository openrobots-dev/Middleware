
#include <r2p/Time.hpp>
#include <ch.h>

namespace r2p {


Time Time::now() {

  register Type t = chTimeNow();
  return ms((t == INFINITEN.raw) ? (t + 1) :
            (t ==  INFINITE.raw) ? (t - 1) : t);
}


} // namespace r2p
