#pragma once

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

  typedef Flasher::Address Address;
  typedef Flasher::Length Length;

  struct AppInfo {
    Length  pgmlen R2P_FLASH_ALIGNED;
    Address pgmadr R2P_FLASH_ALIGNED;
    Length  bsslen R2P_FLASH_ALIGNED;
    Address bssadr R2P_FLASH_ALIGNED;
    Length  datalen R2P_FLASH_ALIGNED;
    Address dataadr R2P_FLASH_ALIGNED;
    Address datapgmadr R2P_FLASH_ALIGNED;
    Length  stacklen R2P_FLASH_ALIGNED;
    Address threadadr R2P_FLASH_ALIGNED;
    char    name[NamingTraits<Node>::MAX_LENGTH] R2P_FLASH_ALIGNED;
  } R2P_FLASH_ALIGNED;

  struct FlashLayout {
    Length  numapps R2P_FLASH_ALIGNED;
    Address freeadr R2P_FLASH_ALIGNED;
    AppInfo infos[MAX_APPS] R2P_FLASH_ALIGNED;
  } R2P_FLASH_ALIGNED;

private:
  State state;
  Flasher flasher;
  Length numapps;
  Address baseadr;
  Address freeadr;
  AppInfo tempinfo;

public:
  void set_page_buffer(Flasher::Data flash_page_buf[]);

  bool process(const BootloaderMsg &request_msg, BootloaderMsg &response_msg);

private:
  bool process_ihex(const IhexRecord &record);
  bool compute_addresses();
  bool update_layout(const AppInfo *last_infop = NULL);

public:
  Bootloader(Flasher::Data *flash_page_bufp);

public:
  static Bootloader instance;
  static const volatile FlashLayout flash_layout;
};


} // namespace r2p
