
#include <r2p/BootloaderMsg.hpp>
#include <cstring>

namespace r2p {


uint8_t IhexRecord::compute_checksum() const {

  uint8_t cs = static_cast<uint8_t>(offset >> 8) +
               static_cast<uint8_t>(offset & 0xFF) + type;
  for (uint8_t i = 0; i < count; ++i) {
    cs += data[i];
  }
  return ~cs + 1;
}


void BootloaderMsg::cleanup() {

  SysLock::acquire();
  checksum = 0;
  type = 0;
  seqn = 0;
  memset(raw_payload, 0, MAX_PAYLOAD_LENGTH);
  SysLock::release();
}


uint8_t BootloaderMsg::compute_checksum() const {

  uint8_t cs = type + static_cast<uint8_t>(seqn >> 8) +
                      static_cast<uint8_t>(seqn);
  for (unsigned i = 0; i < MAX_PAYLOAD_LENGTH; ++i) {
    cs += raw_payload[i];
  }
  return ~cs + 1;
}


} // namespace r2p
