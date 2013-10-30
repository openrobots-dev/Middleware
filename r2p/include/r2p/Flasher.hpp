#pragma once

#include <r2p/common.hpp>
#include <r2p/impl/Flasher_.hpp>

namespace r2p {

#if !defined(R2P_FLASH_ALIGNED) || defined(__DOXYGEN__)
#define R2P_FLASH_ALIGNED   __attribute__((aligned(Flasher::WORD_ALIGNMENT)))
#endif


class Flasher : private Uncopyable {
public:
  typedef Flasher_::Data    Data;
  typedef Flasher_::PageID  PageID;
  typedef Flasher_::Length  Length;

  enum { BASE_ADDRESS       = Flasher_::BASE_ADDRESS };
  enum { PAGE_SIZE          = Flasher_::PAGE_SIZE };
  enum { PROGRAM_ALIGNMENT  = Flasher_::PROGRAM_ALIGNMENT };
  enum { WORD_ALIGNMENT     = Flasher_::WORD_ALIGNMENT };

private:
  Flasher_ impl;

public:
  void set_page_buffer(Data page_buf[]);

  void begin();
  bool end();
  bool flash_aligned(const Data *address, const Data *bufp, size_t buflen);
  bool erase_aligned(const Data *address, size_t length);
  bool flash(const uint8_t *address, const uint8_t *bufp, size_t buflen);
  bool erase(const uint8_t *address, size_t length);

public:
  Flasher(Data page_buf[]);

public:
  static bool is_aligned(const void *ptr);
  static bool is_within_bounds(const void *ptr);
  static bool is_erased(PageID page);
  static bool erase(PageID page);
  static int compare(PageID page, const Data *bufp);
  static bool read(PageID page, Data *bufp);
  static bool write(PageID page, const Data *bufp);
  static bool write_if_needed(PageID page, const Data *bufp);

  static const uint8_t *address_of(PageID page);
  static PageID page_of(const uint8_t *address);
  static const uint8_t *align_prev(const uint8_t *address);
  static const uint8_t *align_next(const uint8_t *address);
  static Length align_prev(Length length);
  static Length align_next(Length length);

  static const uint8_t *get_program_start();
  static const uint8_t *get_program_end();
  static const uint8_t *get_ram_start();
  static const uint8_t *get_ram_end();
  static size_t get_program_length();
  static size_t get_ram_length();
  static bool is_program(const uint8_t *address);
  static bool is_ram(const uint8_t *address);

  static void jump_to(const uint8_t *address);
};


inline
bool Flasher::is_aligned(const void *ptr) {

  return Flasher_::is_aligned(ptr);
}


inline
bool Flasher::is_within_bounds(const void *ptr) {

  return Flasher_::is_within_bounds(ptr);
}


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
bool Flasher::flash_aligned(const Data *address, const Data *bufp,
                            size_t buflen) {

  return impl.flash_aligned(address, bufp, buflen);
}


inline
bool Flasher::erase_aligned(const Data *address, size_t length) {

  return impl.erase_aligned(address, length);
}


inline
bool Flasher::flash(const uint8_t *address, const uint8_t *bufp,
                    size_t buflen) {

  return impl.flash(address, bufp, buflen);
}


inline
bool Flasher::erase(const uint8_t *address, size_t length) {

  return impl.erase(address, length);
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
const uint8_t *Flasher::address_of(PageID page) {

  return Flasher_::address_of(page);
}


inline
Flasher::PageID Flasher::page_of(const uint8_t *address) {

  return Flasher_::page_of(address);
}


inline
const uint8_t *Flasher::align_prev(const uint8_t *address) {

  return Flasher_::align_prev(address);
}


inline
const uint8_t *Flasher::align_next(const uint8_t *address) {

  return Flasher_::align_next(address);
}


inline
Flasher::Length Flasher::align_prev(Length length) {

  return reinterpret_cast<Length>(
    Flasher_::align_prev(reinterpret_cast<const uint8_t *>(length))
  );
}


inline
Flasher::Length Flasher::align_next(Length length) {

  return reinterpret_cast<Length>(
    Flasher_::align_next(reinterpret_cast<const uint8_t *>(length))
  );
}


inline
const uint8_t *Flasher::get_program_start() {

  return Flasher_::get_program_start();
}


inline
const uint8_t *Flasher::get_program_end() {

  return Flasher_::get_program_end();
}


inline
const uint8_t *Flasher::get_ram_start() {

  return Flasher_::get_ram_start();
}


inline
const uint8_t *Flasher::get_ram_end() {

  return Flasher_::get_ram_end();
}


inline
size_t Flasher::get_program_length() {

  return compute_chunk_size(get_program_start(), get_program_end());
}


inline
size_t Flasher::get_ram_length() {

  return compute_chunk_size(get_ram_start(), get_ram_end());
}


inline
bool Flasher::is_program(const uint8_t *address) {

  return address >= get_program_start() && address < get_program_end();
}


inline
bool Flasher::is_ram(const uint8_t *address) {

  return address >= get_ram_start() && address < get_ram_end();
}


inline
void Flasher::jump_to(const uint8_t *address) {

  Flasher_::jump_to(address);
}


} // namespace r2p
