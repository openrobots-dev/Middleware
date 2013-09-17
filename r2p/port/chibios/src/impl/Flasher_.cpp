
#include <ch.h>
#include <hal.h>
#include <r2p/impl/Flasher_.hpp>

namespace r2p {


inline
static void flash_lock(void) {

  FLASH->CR |= FLASH_CR_LOCK;
}


static bool flash_unlock() {

  // Check if unlock is really needed
  if ((FLASH->CR & FLASH_CR_LOCK) == 0) return true;

  // Write magic unlock sequence
  FLASH->KEYR = 0x45670123;
  FLASH->KEYR = 0xCDEF89AB;

  return (FLASH->CR & FLASH_CR_LOCK) == 0;
}


inline
static void flash_busy_wait() {

  while (FLASH->SR & FLASH_SR_BSY) {}
}


void Flasher_::set_page_buffer(Data page_buf[]) {

  R2P_ASSERT(page_buf != NULL);

  page_bufp = page_buf;
}


bool Flasher_::is_erased(PageID page) {

  volatile const Data *const startp =
    reinterpret_cast<volatile const Data *>(address_of(page));
  volatile const Data *const stopp =
    reinterpret_cast<volatile const Data *>(address_of(page + 1));

  // Cycle through the whole page and check for default set bits
  for (volatile const Data *datap = startp; datap < stopp; ++datap) {
    if (*datap != ERASED_WORD) return false;
  }
  return true;
}


bool Flasher_::erase(PageID page) {

  // Unlock flash for write access
  if (!flash_unlock()) return false;
  flash_busy_wait();

  // Start deletion of page.
  FLASH->CR |= FLASH_CR_PER;
  FLASH->AR = reinterpret_cast<uint32_t>(address_of(page));
  FLASH->CR |= FLASH_CR_STRT;
  flash_busy_wait();

  // Page erase flag does not clear automatically.
  FLASH->CR &= !FLASH_CR_PER;

  flash_lock();
  return is_erased(page);
}


int Flasher_::compare(PageID page, volatile const Data *bufp) {

  volatile const Data *const flashp =
    reinterpret_cast<volatile const Data *>(address_of(page));

  bool identical = true;

  for (size_t pos = 0; pos < PAGE_SIZE / sizeof(Data); ++pos) {
    if (flashp[pos] != bufp[pos]) {
      // Keep track if the buffer is identical to page -> mark not identical
      identical = false;

      // Not identical, and not erased, needs erase
      if (flashp[pos] != ERASED_WORD) return -1;
    }
  }

  // Page is not identical, but no page erase is needed to write
  if (!identical) return 1;

  // Page is identical. No write is needed
  return 0;
}


bool Flasher_::read(PageID page, Data *bufp) {

  memcpy(static_cast<void *>(bufp),
         reinterpret_cast<const void *>(address_of(page)),
         PAGE_SIZE);
  return true;
}


bool Flasher_::write(PageID page, volatile const Data *bufp) {

  if (!check_bounds(address_of(page), PAGE_SIZE, get_program_start(),
                    get_program_length())) {
    return false;
  }

  volatile Data *const flashp = reinterpret_cast<volatile Data *>(
    reinterpret_cast<uintptr_t>(address_of(page))
  );

  chSysDisable();

  // Unlock flash for write access
  if (!flash_unlock()) {
    flash_lock();
    chSysEnable();
    return false;
  }
  flash_busy_wait();

  for (size_t pos = 0; pos < PAGE_SIZE / sizeof(Data); ++pos) {
    // Enter flash programming mode
    FLASH->CR |= FLASH_CR_PG;

    // Write half-word to flash
    flashp[pos] = bufp[pos];
    flash_busy_wait();

    // Exit flash programming mode
    FLASH->CR &= ~FLASH_CR_PG;

    // Check for flash error
    if (flashp[pos] != bufp[pos]) {
      flash_lock();
      chSysEnable();
      return false;
    }
  }

  flash_lock();
  chSysEnable();
  return true;

}


bool Flasher_::write_if_needed(PageID page, volatile const Data *bufp) {

  // TODO: Only write on pages in the user area

  // Don't do anything if pages are identical
  register int comparison = compare(page, bufp);
  if (comparison == 0) return true;

  // Page needs erase
  if (comparison < 0) {
    if (!erase(page)) return false;
  }

  // Write data
  return write(page, bufp);
}


void Flasher_::begin() {

  R2P_ASSERT(page_modified == false);

  page = INVALID_PAGE;
}


bool Flasher_::end() {

  // Write back last buffer if it is tainted
  if (page_modified) {
    page_modified = false;
    return write_if_needed(page, page_bufp);
  }
  return true;
}


bool Flasher_::flash(const uint8_t *address, const Data *bufp, size_t buflen) {

  bool writeback = false;

  // Process all given words
  while (buflen > 0) {
    PageID old_page = page;
    page = page_of(address);
    ptrdiff_t offset = reinterpret_cast<ptrdiff_t>(address) -
                       reinterpret_cast<ptrdiff_t>(address_of(page));
    size_t length = (PAGE_SIZE - offset);

    // Read back new page if page has changed
    if (old_page != page) {
      if (page_modified) {
        if (!write_if_needed(old_page, page_bufp)) return false;
      }
      if (!read(page, page_bufp)) return false;
      page_modified = false;
    }

    // Process no more bytes than remaining
    if (length > buflen) {
      length = buflen;
    } else {
      writeback = true;
    }

    // Copy buffer into page buffer and mark as tainted
    memcpy(&page_bufp[offset / sizeof(Data)], bufp, length);
    page_modified = true;
    buflen -= length;

    // Writeback buffer if needed
    if (writeback) {
      if (!write_if_needed(page, page_bufp)) return false;
      writeback = false;
    }
  }
  return true;
}


bool Flasher_::erase(const uint8_t *address, size_t length) {

  static const Data erased_word_value = ERASED_WORD;

  // const uint8_t *and length must be aligned
  R2P_ASSERT((reinterpret_cast<uintptr_t>(address) &
             bit_mask(sizeof(Data))) == 0);
  R2P_ASSERT((length & bit_mask(sizeof(Data))) == 0);

  while (length > 0) {
    flash(address, &erased_word_value, sizeof(Data));
    address += sizeof(Data);
    length -= sizeof(Data);
  }
  return true;
}


Flasher_::Flasher_(Data page_buf[])
:
  page_bufp(page_buf),
  page_modified(false),
  page(0)
{}


void Flasher_::jump_to(const uint8_t *address) {

  typedef void (*Proc)(void);

  // Load jump address into function pointer
  Proc proc = reinterpret_cast<Proc>(
    reinterpret_cast<const uint32_t *>(address)[1]
  );

  // Reset all interrupts to default
  chSysDisable();

  // Clear pending interrupts just to be on the safe side
  SCB_ICSR = ICSR_PENDSVCLR;

  // Disable all interrupts
  for (unsigned i = 0; i < 8; ++i) {
    NVIC->ICER[i] = NVIC->IABR[i];
  }

  // Set stack pointer as in application's vector table
  __set_MSP((reinterpret_cast<const uint32_t *>(address))[0]);
  proc();
}


} // namespace r2p
