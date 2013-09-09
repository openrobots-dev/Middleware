#pragma once

#include <r2p/common.hpp>
#include <r2p/BootloaderMsg.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/Flasher.hpp>
#include <r2p/Thread.hpp>

namespace r2p {

template<typename MessageType> class Publisher;
template<typename MessageType> class Subscriber;


class Bootloader : private Uncopyable {
public:

  struct FlashAppInfo {
    const FlashAppInfo  *nextp      R2P_FLASH_ALIGNED;
    size_t              pgmlen      R2P_FLASH_ALIGNED;
    size_t              bsslen      R2P_FLASH_ALIGNED;
    size_t              datalen     R2P_FLASH_ALIGNED;
    const uint8_t       *pgmp       R2P_FLASH_ALIGNED;
    const uint8_t       *bssp       R2P_FLASH_ALIGNED;
    const uint8_t       *datap      R2P_FLASH_ALIGNED;
    const uint8_t       *datapgmp   R2P_FLASH_ALIGNED;
    const uint8_t       *cfgp       R2P_FLASH_ALIGNED;
    size_t              stacklen    R2P_FLASH_ALIGNED;
    Thread::Function    threadf     R2P_FLASH_ALIGNED;
    char                name[NamingTraits<Node>::MAX_LENGTH]
                        R2P_FLASH_ALIGNED;

    bool has_name(const char *namep) const {
      return 0 == strncmp(name, namep, NamingTraits<Node>::MAX_LENGTH);
    }
  } R2P_FLASH_ALIGNED;

  struct FlashSummary {
    const uint8_t       *freep      R2P_FLASH_ALIGNED;
    const FlashAppInfo  *headp      R2P_FLASH_ALIGNED;
  } R2P_FLASH_ALIGNED;

  struct FullAppInfo {
    const FlashAppInfo  *nextp;
    size_t              pgmlen;
    size_t              bsslen;
    size_t              datalen;
    const uint8_t       *pgmp;
    const uint8_t       *bssp;
    const uint8_t       *datap;
    const uint8_t       *datapgmp;
    const uint8_t       *cfgp;
    size_t              stacklen;
    Thread::Function    threadf;
    char                name[NamingTraits<Node>::MAX_LENGTH];
  };

private:
  const uint8_t *basep;
  Flasher flasher;

  Publisher<BootMsg> *pubp;
  Subscriber<BootMsg> *subp;
  BootMsg *master_msgp;
  BootMsg *slave_msgp;

public:
  void spin_loop();

private:
  void alloc();
  void alloc(BootMsg::TypeEnum type);
  void publish();
  void fetch();
  bool fetch(BootMsg::TypeEnum expected_type);
  void release();

  void do_loader();
  void do_appinfo();
  void do_getparam();
  void do_setparam();
  void do_ack();
  void do_error();
  void do_release_error();

  bool process_ihex(const IhexRecord &record,
                    const BootMsg::LinkingSetup &setup,
                    const BootMsg::LinkingAddresses &addresses);
  bool compute_addresses(const BootMsg::LinkingSetup &setup,
                         BootMsg::LinkingAddresses &addresses);

  const uint8_t *get_app_cfg(const char *appname) const;
  uint8_t *reserve_ram(size_t length);
  uint8_t *unreserve_ram(size_t length);
  void begin_write();
  void end_write();
  bool write(const void *dstp, const void *srcp, size_t length);
  bool write_summary(const FlashSummary &summary);
  bool write_appinfo(const FlashAppInfo *flashinfop,
                     const BootMsg::LinkingSetup &setup,
                     const BootMsg::LinkingAddresses &addresses,
                     const BootMsg::LinkingOutcome &outcome);
  bool remove_last();
  bool remove_all();
  bool spawn(const char *appname);
  bool spawn_all();

public:
  Bootloader(Flasher::Data *flash_page_bufp,
             Publisher<BootMsg> &pub,
             Subscriber<BootMsg> &sub);

public:
  static const FlashAppInfo *get_last_app();

public:
  static const FlashSummary app_summary R2P_FLASH_ALIGNED;
} R2P_FLASH_ALIGNED;


inline
void Bootloader::alloc(BootMsg::TypeEnum type) {

  alloc();
  slave_msgp->type = type;
}


inline
bool Bootloader::fetch(BootMsg::TypeEnum expected_type) {

  fetch();
  return master_msgp->type == expected_type;
}


inline
void Bootloader::begin_write() {

  flasher.begin();
}


inline
void Bootloader::end_write() {

  bool success; (void)success;
  success = flasher.end();
  R2P_ASSERT(success);
}


inline
bool Bootloader::write_summary(const FlashSummary &summary) {

  return write(const_cast<FlashSummary *>(&app_summary), &summary,
               sizeof(FlashSummary));
}


inline
bool Bootloader::write(const void *dstp, const void *srcp, size_t length) {

  return flasher.flash(reinterpret_cast<Flasher::Address>(dstp),
                       reinterpret_cast<const Flasher::Data *>(srcp),
                       length);
}


} // namespace r2p
