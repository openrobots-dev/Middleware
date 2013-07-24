
#ifndef __R2P__BOOTLOADER_HPP__
#define __R2P__BOOTLOADER_HPP__

#include <r2p/common.hpp>
#include <r2p/BootloaderMsg.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/Flasher.hpp>

namespace r2p {

#if !defined(R2P_MAX_APPS) || defined(__DOXYGEN__)
#define R2P_MAX_APPS    16
#endif


class Bootloader : private Uncopyable {
public:
  enum { MAX_APPS = R2P_MAX_APPS };

  enum State {
    AWAITING_REQUEST = 0,
    COMPUTING_RESPONSE,
    RECEIVING_IHEX
  };

  struct AppInfo {
    Flasher::Length   pgmlen;
    Flasher::Address  pgmadr;
    Flasher::Length   bsslen;
    Flasher::Address  bssadr;
    Flasher::Length   datalen;
    Flasher::Address  dataadr;
    Flasher::Address  datapgmadr;
    Flasher::Length   stacklen;
    Flasher::Address  threadadr;
    char              name[NamingTraits<Node>::MAX_LENGTH] R2P_FLASH_ALIGNED;
    uint8_t           namelen;
  } R2P_FLASH_ALIGNED;

  struct FlashLayout {
    uint32_t          numapps R2P_FLASH_ALIGNED;
    Flasher::Address  freeadr R2P_FLASH_ALIGNED;
    AppInfo           infos[MAX_APPS] R2P_FLASH_ALIGNED;
  } R2P_FLASH_ALIGNED;

private:
  State state;
  Flasher flasher;
  uint32_t numapps;
  Flasher::Address baseadr;
  Flasher::Address freeadr;
  AppInfo tempinfo;

public:
  bool process(const BootloaderMsg &request_msg, BootloaderMsg &response_msg);

private:
  bool process_ihex(const IhexRecord &record);
  bool compute_addresses();
  bool update_layout(const AppInfo *last_infop = NULL);

private:
  Bootloader(Flasher::Data *flash_page_bufp);

public:
  static Bootloader instance;
  static FlashLayout flash_layout;
};


} // namespace r2p
#endif // __R2P__BOOTLOADER_HPP__
