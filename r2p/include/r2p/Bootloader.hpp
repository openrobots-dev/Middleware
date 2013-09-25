#pragma once

#include <r2p/common.hpp>
#include <r2p/BootMsg.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/Flasher.hpp>
#include <r2p/Thread.hpp>

#if !defined(R2P_USE_BOOTLOADER) || defined(__DOXYGEN__)
#define R2P_USE_BOOTLOADER  0
#endif

#if R2P_USE_BOOTLOADER
namespace r2p {

template<typename MessageType> class Publisher;
template<typename MessageType> class Subscriber;


class Bootloader : private Uncopyable {
private:
  enum { SIGNATURE = 0x1D2C5A69 };

public:

  class Iterator;

  struct FlashAppInfo {
    union Flags {
      uint16_t raw;
      struct {
        unsigned  enabled     : 1;
      };
    };

    typedef void (*Proc)();

    uint32_t            signature   R2P_FLASH_ALIGNED;
    size_t              pgmlen      R2P_FLASH_ALIGNED;
    size_t              bsslen      R2P_FLASH_ALIGNED;
    size_t              datalen     R2P_FLASH_ALIGNED;
    const uint8_t       *bssp       R2P_FLASH_ALIGNED;
    const uint8_t       *datap      R2P_FLASH_ALIGNED;
    const uint8_t       *cfgp       R2P_FLASH_ALIGNED;
    size_t              cfglen      R2P_FLASH_ALIGNED;
    size_t              stacklen    R2P_FLASH_ALIGNED;
    Proc                mainf       R2P_FLASH_ALIGNED;
    const uint8_t       *ctorsp     R2P_FLASH_ALIGNED;
    size_t              ctorslen    R2P_FLASH_ALIGNED;
    const uint8_t       *dtorsp     R2P_FLASH_ALIGNED;
    size_t              dtorslen    R2P_FLASH_ALIGNED;
    const char          name[NamingTraits<Node>::MAX_LENGTH]
                        R2P_FLASH_ALIGNED;
    Flags               flags       R2P_FLASH_ALIGNED;


    static const uint8_t *compute_program(const FlashAppInfo *infop) {
      return reinterpret_cast<const uint8_t *>(infop) +
             Flasher::align_next(sizeof(FlashAppInfo));
    }

    static const uint8_t *compute_data_program(const FlashAppInfo *infop,
                                               size_t pgmlen, size_t datalen) {
      if (datalen > 0) {
        return reinterpret_cast<const uint8_t *>(infop) +
               Flasher::align_next(sizeof(FlashAppInfo)) +
               Flasher::align_next(pgmlen);
      } else {
        return NULL;
      }
    }

    static const FlashAppInfo *compute_next(const FlashAppInfo *infop,
                                            size_t pgmlen, size_t datalen) {
      return reinterpret_cast<const FlashAppInfo *>(
        reinterpret_cast<const uint8_t *>(infop) +
        Flasher::align_next(sizeof(FlashAppInfo)) +
        Flasher::align_next(pgmlen) +
        Flasher::align_next(datalen)
      );
    }

    const uint8_t *get_program() const {
      return compute_program(this);
    }

    const uint8_t *get_data_program() const {
      return compute_data_program(this, pgmlen, datalen);
    }

    const FlashAppInfo *get_next() const {
      return compute_next(this, pgmlen, datalen);
    }

    bool is_valid() const {
      return signature == SIGNATURE &&
             Flasher::is_within_bounds(reinterpret_cast<const uint8_t *>(this));
    }

    bool has_name(const char *namep) const {
      return 0 == strncmp(name, namep, NamingTraits<Node>::MAX_LENGTH);
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
  void do_error(uint32_t line, const char *textp = "");
  void do_release_error(uint32_t line, const char *textp = "");

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
  bool remove_last();
  bool remove_all();

public:
  Bootloader(Flasher::Data *flash_page_bufp,
             Publisher<BootMsg> &pub, Subscriber<BootMsg> &sub);

private:
  static const Iterator begin();
  static const Iterator end();
  static size_t get_num_apps();
  static const uint8_t *get_free_address();
  static const FlashAppInfo *get_free_app();
  static const FlashAppInfo *get_last_app();
  static const FlashAppInfo *get_app(const char *appname);
  static bool compute_addresses(const BootMsg::LinkingSetup &setup,
                                BootMsg::LinkingAddresses &addresses);
  static uint8_t *reserve_ram(size_t length);
  static uint8_t *unreserve_ram(size_t length);
  static bool launch(const char *appname);
  static Thread::Return app_threadf_wrapper(Thread::Argument appinfop);

public:
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

    operator const FlashAppInfo * () const {
      return curp;
    }

    operator const FlashAppInfo & () const {
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

  return flasher.flash(reinterpret_cast<const uint8_t *>(dstp),
                       reinterpret_cast<const Flasher::Data *>(srcp),
                       length);
}


inline
const Bootloader::Iterator Bootloader::begin() {

  return Iterator(reinterpret_cast<const FlashAppInfo *>(
    Flasher::get_program_start()
  ));
}


inline
const Bootloader::Iterator Bootloader::end() {

  return Iterator(reinterpret_cast<const FlashAppInfo *>(
    &an_invalid_signature
  ));
}


inline
const uint8_t *Bootloader::get_free_address() {

  return reinterpret_cast<const uint8_t *>(get_free_app());
}


} // namespace r2p
#endif // R2P_USE_BOOTLOADER
