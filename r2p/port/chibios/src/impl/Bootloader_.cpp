
#include <r2p/Bootloader.hpp>

#if R2P_USE_BOOTLOADER
namespace r2p {


uint8_t *Bootloader::reserve_ram(size_t length) {

  return reinterpret_cast<uint8_t *>(chCoreReserve(length));
}


uint8_t *Bootloader::unreserve_ram(size_t length) {

  return reinterpret_cast<uint8_t *>(chCoreUnreserve(length));
}


} // namespace r2p
#endif // R2P_USE_BOOTLOADER
