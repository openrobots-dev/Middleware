
#include <r2p/BootloaderMsg.hpp>
#include <cstring>

namespace r2p {


uint8_t IhexRecord::compute_checksum() const {

  uint8_t cs = count + static_cast<uint8_t>(offset >> 8) +
               static_cast<uint8_t>(offset & 0xFF) + type;
  for (uint8_t i = 0; i < count; ++i) {
    cs += data[i];
  }
  return ~cs + 1;
}


} // namespace r2p
