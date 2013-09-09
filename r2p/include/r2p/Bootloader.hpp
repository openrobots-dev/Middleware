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
private:
  enum { SIGNATURE = 0x1D2C5A69 };

public:

  class Iterator;

  struct FlashAppInfo {
    uint32_t            signature   R2P_FLASH_ALIGNED;
    size_t              pgmlen      R2P_FLASH_ALIGNED;
    size_t              bsslen      R2P_FLASH_ALIGNED;
    size_t              datalen     R2P_FLASH_ALIGNED;
    const uint8_t       *bssp       R2P_FLASH_ALIGNED;
    const uint8_t       *datap      R2P_FLASH_ALIGNED;
    const uint8_t       *cfgp       R2P_FLASH_ALIGNED;
    size_t              stacklen    R2P_FLASH_ALIGNED;
    Thread::Function    threadf     R2P_FLASH_ALIGNED;
    const char          name[NamingTraits<Node>::MAX_LENGTH]
                        R2P_FLASH_ALIGNED;

    const uint8_t *get_program() const {
      return reinterpret_cast<const uint8_t *>(this) +
             Flasher::align_next(sizeof(FlashAppInfo));
    }

    const uint8_t *get_data_program() const {
      return reinterpret_cast<const uint8_t *>(this) +
             Flasher::align_next(sizeof(FlashAppInfo)) +
             Flasher::align_next(pgmlen);
    }

    bool is_valid() const {
      return signature == SIGNATURE &&
             reinterpret_cast<Flasher::Address>(this) >=
             Flasher::get_program_start() &&
             reinterpret_cast<Flasher::Address>(this) <=
             (Flasher::get_program_end() - sizeof(FlashAppInfo));
    }

    bool has_name(const char *namep) const {
      return 0 == strncmp(name, namep, NamingTraits<Node>::MAX_LENGTH);
    }

    const FlashAppInfo *get_next() const {
      return reinterpret_cast<const FlashAppInfo *>(
        reinterpret_cast<const uint8_t *>(this) +
        Flasher::align_next(sizeof(FlashAppInfo)) +
        Flasher::align_next(pgmlen) +
        Flasher::align_next(datalen)
      );
    }
  } R2P_FLASH_ALIGNED;

private:
  const uint8_t *basep;
  Flasher flasher;

  Publisher<BootMsg> *pubp;
  Subscriber<BootMsg> *subp;
  BootMsg *master_msgp;
  BootMsg *slave_msgp;

private:
  static const uint32_t an_invalid_signature;

public:
  bool remove_last();
  bool remove_all();

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

  void begin_write();
  void end_write();
  bool write(const void *dstp, const void *srcp, size_t length);
  bool write_appinfo(const FlashAppInfo *flashinfop,
                     const BootMsg::LinkingSetup &setup,
                     const BootMsg::LinkingAddresses &addresses,
                     const BootMsg::LinkingOutcome &outcome);
  bool process_ihex(const IhexRecord &record,
                    const BootMsg::LinkingSetup &setup,
                    const BootMsg::LinkingAddresses &addresses);

public:
  Bootloader(Flasher::Data *flash_page_bufp,
             Publisher<BootMsg> &pub, Subscriber<BootMsg> &sub);

private:
  static const Iterator begin();
  static const Iterator end();
  static size_t get_num_apps();
  static const uint8_t *get_free_address();
  static const FlashAppInfo *get_app(const char *appname);
  static bool compute_addresses(const BootMsg::LinkingSetup &setup,
                                BootMsg::LinkingAddresses &addresses);
  static uint8_t *reserve_ram(size_t length);
  static uint8_t *unreserve_ram(size_t length);
  static bool launch(const char *appname);
  static bool launch_all();

public:

  class Iterator {
    friend class Bootloader;

  private:
    const FlashAppInfo *curp;

  public:
    Iterator(const Iterator &rhs) : curp(rhs.curp) {}

  private:
    Iterator(const FlashAppInfo *beginp) : curp(beginp) {}

  private:
    Iterator &operator = (const Iterator &rhs) {
      curp = rhs.curp;
      return *this;
    }

  public:
    const FlashAppInfo *operator -> () const {
      return curp;
    }

    const FlashAppInfo &operator * () const {
      return *curp;
    }

    Iterator &operator ++ () {
      curp = curp->get_next();
      return *this;
    }

    const Iterator operator ++ (int) {
      Iterator old(*this);
      curp = curp->get_next();
      return old;
    }

    bool operator == (const Iterator &rhs) const {
      return this->curp->is_valid() == rhs.curp->is_valid();
    }

    bool operator != (const Iterator &rhs) const {
      return this->curp->is_valid() != rhs.curp->is_valid();
    }
  };

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
bool Bootloader::write(const void *dstp, const void *srcp, size_t length) {

  return flasher.flash(reinterpret_cast<Flasher::Address>(dstp),
                       reinterpret_cast<const Flasher::Data *>(srcp),
                       length);
}


inline
const Bootloader::Iterator Bootloader::begin() {

  return Iterator(reinterpret_cast<const FlashAppInfo *>(
    Flasher::get_program_start()
  ));
}


} // namespace r2p
