#pragma once

#include <r2p/common.hpp>

extern "C" {


extern const uint8_t __program_start__[];
extern const uint8_t __program_end__[];

extern const uint8_t __ram_start__[];
extern const uint8_t __ram_end__[];

extern const uint8_t __layout_start__[]; // TODO: remove
extern const uint8_t __layout_end__[]; // TODO: remove


}

namespace r2p {

#if !defined(FLASH_BASE_ADDRESS) || defined (__DOXYGEN__)
#define FLASH_BASE_ADDRESS  0x08000000
#endif

#if !defined(R2P_FLASH_ALIGNED)
#define R2P_FLASH_ALIGNED   __attribute__((aligned(Flasher_::WORD_ALIGNMENT)))
#endif


class Flasher_ : private Uncopyable {
public:
  typedef uint16_t      Data;
  typedef uint16_t      PageID;
  typedef size_t        Length;

  enum { BASE_ADDRESS       = FLASH_BASE_ADDRESS };
  enum { PAGE_SIZE          = 1024 };
  enum { PROGRAM_ALIGNMENT  = 16 };
  enum { WORD_ALIGNMENT     = sizeof(Data) };
  enum { ERASED_BYTE        = 0xFF };
  enum { ERASED_WORD        = 0xFFFF };
  enum { INVALID_PAGE       = 0xFFFF };

private:
  Data *page_bufp;
  bool page_modified;
  PageID page;

public:
  void set_page_buffer(Data page_buf[]);

  void begin();
  bool end();
  bool flash_aligned(const Data *address, const Data *bufp, size_t buflen);
  bool erase_aligned(const Data *address, size_t length);
  bool flash(const uint8_t *address, const uint8_t *bufp, size_t buflen);
  bool erase(const uint8_t *address, size_t length);

public:
  Flasher_(Data page_buf[]);

public:
  static bool is_aligned(const void *ptr);
  static bool is_within_bounds(const void *address);
  static bool is_erased(PageID page);
  static bool erase(PageID page);
  static int compare(PageID page, volatile const Data *bufp);
  static bool read(PageID page, Data *bufp);
  static bool write(PageID page, volatile const Data *bufp);
  static bool write_if_needed(PageID page, volatile const Data *bufp);

  static const uint8_t *address_of(PageID page);
  static PageID page_of(const uint8_t *address);
  static const uint8_t *align_prev(const uint8_t *address);
  static const uint8_t *align_next(const uint8_t *address);

  static const uint8_t *get_program_start();
  static const uint8_t *get_program_end();
  static size_t get_program_length();
  static const uint8_t *get_ram_start();
  static const uint8_t *get_ram_end();
  static size_t get_ram_length();

  static void jump_to(const uint8_t *address);

public:
  static const Data erased_word R2P_FLASH_ALIGNED;
};


inline
bool Flasher_::is_aligned(const void *ptr) {

  return (reinterpret_cast<uintptr_t>(ptr) & (WORD_ALIGNMENT - 1)) == 0;
}


inline
bool Flasher_::is_within_bounds(const void *ptr) {

  return reinterpret_cast<const uint8_t *>(ptr) >= get_program_start() &&
         reinterpret_cast<const uint8_t *>(ptr) < get_program_end();
}


inline
const uint8_t *Flasher_::address_of(PageID page) {

  return reinterpret_cast<const uint8_t *>(
    BASE_ADDRESS + static_cast<uintptr_t>(page) * PAGE_SIZE
  );
}


inline
Flasher_::PageID Flasher_::page_of(const uint8_t *address) {

  return static_cast<PageID>(
    (reinterpret_cast<uintptr_t>(address) - BASE_ADDRESS) / PAGE_SIZE
  );
}


inline
const uint8_t *Flasher_::align_prev(const uint8_t *address) {

  return reinterpret_cast<const uint8_t *>(
    reinterpret_cast<uintptr_t>(address) & ~(PROGRAM_ALIGNMENT - 1)
  );
}


inline
const uint8_t *Flasher_::align_next(const uint8_t *address) {

  return align_prev(address + (PROGRAM_ALIGNMENT - 1));
}


inline
const uint8_t *Flasher_::get_program_start() {

  // Align to the next page, to avoid overwriting of the static firmware
  return reinterpret_cast<const uint8_t *>((reinterpret_cast<uintptr_t>(
    __program_start__) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1)
  );
}


inline
const uint8_t *Flasher_::get_program_end() {

  return align_prev(__program_end__);
}


inline
size_t Flasher_::get_program_length() {

  return compute_chunk_size(get_program_start(), get_program_end());
}


inline
const uint8_t *Flasher_::get_ram_start() {

  return __ram_start__;
}


inline
const uint8_t *Flasher_::get_ram_end() {

  return __ram_end__;
}


inline
size_t Flasher_::get_ram_length() {

  return compute_chunk_size(get_ram_start(), get_ram_end());
}


} // namespace r2p
