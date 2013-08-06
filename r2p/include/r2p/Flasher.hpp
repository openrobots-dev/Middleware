#pragma once

#include <r2p/common.hpp>
#include <r2p/impl/Flasher_.hpp>

namespace r2p {

#define R2P_FLASH_ALIGNED \
  __attribute__((aligned(sizeof(r2p::Flasher::Data))))


class Flasher : private Uncopyable {
public:
  enum { BASE_ADDRESS       = Flasher_::BASE_ADDRESS };
  enum { PAGE_SIZE          = Flasher_::PAGE_SIZE };
  enum { PROGRAM_ALIGNMENT  = Flasher_::PROGRAM_ALIGNMENT };
  enum { WORD_ALIGNMENT     = Flasher_::WORD_ALIGNMENT };

  typedef Flasher_::Address Address;
  typedef Flasher_::Data Data;
  typedef Flasher_::PageID PageID;
  typedef Flasher_::Length Length;

private:
  Flasher_ impl;

public:
  void set_page_buffer(Data page_buf[]);

  void begin();
  bool end();
  bool flash(Address address, const Data *bufp, size_t buflen);

public:
  Flasher(Data page_buf[]);

public:
  static bool is_erased(PageID page);
  static bool erase(PageID page);
  static int compare(PageID page, const Data *bufp);
  static bool read(PageID page, Data *bufp);
  static bool write(PageID page, const Data *bufp);
  static bool write_if_needed(PageID page, const Data *bufp);

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
void Flasher::set_page_buffer(Data page_buf[]) {

  impl.set_page_buffer(page_buf);
}


inline
void Flasher::begin() {

  impl.begin();
}


inline
bool Flasher::end() {

  return impl.end();
}


inline
bool Flasher::flash(Address address, const Data *bufp, size_t buflen) {

  return impl.flash(address, bufp, buflen);
}


inline
Flasher::Flasher(Data page_buf[])
:
  impl(page_buf)
{}


inline
bool Flasher::is_erased(PageID page) {

  return Flasher_::is_erased(page);
}


inline
bool Flasher::erase(PageID page) {

  return Flasher_::erase(page);
}


inline
int Flasher::compare(PageID page, const Data *bufp) {

  return Flasher_::compare(page, bufp);
}


inline
bool Flasher::read(PageID page, Data *bufp) {

  return Flasher_::read(page, bufp);
}


inline
bool Flasher::write(PageID page, const Data *bufp) {

  return Flasher_::write(page, bufp);
}


inline
bool Flasher::write_if_needed(PageID page, const Data *bufp) {

  return Flasher_::write_if_needed(page, bufp);
}


inline
Flasher::Address Flasher::address_of(PageID page) {

  return Flasher_::address_of(page);
}


inline
Flasher::PageID Flasher::page_of(Address address) {

  return Flasher_::page_of(address);
}


inline
Flasher::Address Flasher::align_prev(Address address) {

  return Flasher_::align_prev(address);
}


inline
Flasher::Address Flasher::align_next(Address address) {

  return Flasher_::align_next(address);
}


inline
Flasher::Address Flasher::get_program_start() {

  return Flasher_::get_program_start();
}


inline
Flasher::Address Flasher::get_program_end() {

  return Flasher_::get_program_end();
}


inline
Flasher::Address Flasher::get_ram_start() {

  return Flasher_::get_ram_start();
}


inline
Flasher::Address Flasher::get_ram_end() {

  return Flasher_::get_ram_end();
}


inline
Flasher::Address Flasher::get_layout_start() {

  return Flasher_::get_layout_start();
}


inline
Flasher::Address Flasher::get_layout_end() {

  return Flasher_::get_layout_end();
}


inline
void Flasher::jump_to(Address address) {

  Flasher_::jump_to(address);
}


} // namespace r2p
