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


class Flasher_ : private Uncopyable {
public:
  typedef const uint8_t *Address;
  typedef uint16_t      Data;
  typedef uint16_t      PageID;
  typedef size_t        Length;

  enum { BASE_ADDRESS       = FLASH_BASE_ADDRESS };
  enum { PAGE_SIZE          = 1024 };
  enum { PROGRAM_ALIGNMENT  = 16 };
  enum { WORD_ALIGNMENT     = sizeof(Data) };

private:
  Data *page_bufp;
  bool page_modified;
  PageID page;

public:
  void set_page_buffer(Data page_buf[]);

  void begin();
  bool end();
  bool flash(Address address, const Data *bufp, size_t buflen);

public:
  Flasher_(Data page_buf[]);

public:
  static bool is_erased(PageID page);
  static bool erase(PageID page);
  static int compare(PageID page, volatile const Data *bufp);
  static bool read(PageID page, Data *bufp);
  static bool write(PageID page, volatile const Data *bufp);
  static bool write_if_needed(PageID page, volatile const Data *bufp);

  static Address address_of(PageID page);
  static PageID page_of(Address address);
  static Address align_prev(Address address);
  static Address align_next(Address address);

  static Address get_program_start();
  static Address get_program_end();
  static Address get_ram_start();
  static Address get_ram_end();
  static Address get_layout_start();
  static Address get_layout_end();

  static void jump_to(Address address);
};


inline
Flasher_::Address Flasher_::address_of(PageID page) {

  return reinterpret_cast<Address>(
    BASE_ADDRESS + static_cast<uintptr_t>(page) * PAGE_SIZE
  );
}


inline
Flasher_::PageID Flasher_::page_of(Address address) {

  return static_cast<PageID>(
    (reinterpret_cast<uintptr_t>(address) - BASE_ADDRESS) / PAGE_SIZE
  );
}


inline
Flasher_::Address Flasher_::align_prev(Address address) {

  return reinterpret_cast<Address>(
    reinterpret_cast<uintptr_t>(address) & ~(PROGRAM_ALIGNMENT - 1)
  );
}


inline
Flasher_::Address Flasher_::align_next(Address address) {

  return align_prev(address + (PROGRAM_ALIGNMENT - 1));
}


inline
Flasher_::Address Flasher_::get_program_start() {

  return reinterpret_cast<Address>(__program_start__);
}


inline
Flasher_::Address Flasher_::get_program_end() {

  return reinterpret_cast<Address>(__program_end__);
}


inline
Flasher_::Address Flasher_::get_ram_start() {

  return reinterpret_cast<Address>(__ram_start__);
}


inline
Flasher_::Address Flasher_::get_ram_end() {

  return reinterpret_cast<Address>(__ram_end__);
}


inline
Flasher_::Address Flasher_::get_layout_start() {

  return reinterpret_cast<Address>(__layout_start__);
}


inline
Flasher_::Address Flasher_::get_layout_end() {

  return reinterpret_cast<Address>(__layout_end__);
}


} // namespace r2p
