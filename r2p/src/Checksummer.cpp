
#include <r2p/Checksummer.hpp>

namespace r2p {


void Checksummer::add(const uint8_t *chunkp, size_t length) {

  while (length-- > 0) {
    add(*chunkp++);
  }
}


} // namespace r2p
