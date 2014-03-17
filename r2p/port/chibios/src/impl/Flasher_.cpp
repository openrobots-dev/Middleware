
#if R2P_USE_BOOTLOADER

#include <ch.h>
#include <hal.h>
#include <r2p/impl/Flasher_.hpp>

namespace r2p {


const Flasher_::Data Flasher_::erased_word R2P_FLASH_ALIGNED =
  Flasher_::ERASED_WORD;


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

  R2P_ASSERT(is_aligned(const_cast<const Data *>(bufp)));

  volatile const Data *const flashp =
    reinterpret_cast<volatile const Data *>(address_of(page));

  bool identical = true;

  for (size_t pos = 0; pos < PAGE_SIZE / WORD_ALIGNMENT; ++pos) {
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

  R2P_ASSERT(is_aligned(bufp));

  memcpy(static_cast<void *>(bufp),
         reinterpret_cast<const void *>(address_of(page)),
         PAGE_SIZE);
  return true;
}


bool Flasher_::write(PageID page, volatile const Data *bufp) {

  R2P_ASSERT(is_aligned(const_cast<const Data *>(bufp)));

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

  for (size_t pos = 0; pos < PAGE_SIZE / WORD_ALIGNMENT; ++pos) {
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

  R2P_ASSERT(is_aligned(const_cast<const Data *>(bufp)));

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


bool Flasher_::flash_aligned(const Data *address, const Data *bufp,
                             size_t buflen) {

  R2P_ASSERT(is_aligned(address));
  R2P_ASSERT(is_aligned(bufp));
  R2P_ASSERT(is_aligned(reinterpret_cast<const void *>(buflen)));

  bool writeback = false;

  // Process all given words
  while (buflen > 0) {
    PageID old_page = page;
    page = page_of(reinterpret_cast<const uint8_t *>(address));
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
    memcpy(&page_bufp[offset / WORD_ALIGNMENT], bufp, length);
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


bool Flasher_::erase_aligned(const Data *address, size_t length) {

  R2P_ASSERT(is_aligned(address));
  R2P_ASSERT(is_aligned(reinterpret_cast<const void *>(length)));

  while (length > 0) {
    flash_aligned(address, &erased_word, WORD_ALIGNMENT);
    address += WORD_ALIGNMENT;
    length -= WORD_ALIGNMENT;
  }
  return true;
}


bool Flasher_::flash(const uint8_t *address, const uint8_t *bufp,
                     size_t buflen) {

  if (buflen == 0) return true;

  Data partial R2P_FLASH_ALIGNED;

  // Write the unaligned buffer beginning
  size_t offset = compute_offset(address, WORD_ALIGNMENT);
  address -= offset;
  partial = *reinterpret_cast<const Data *>(address);
  if (offset + buflen <= WORD_ALIGNMENT) {
    // A single word to write
    memcpy(reinterpret_cast<uint8_t *>(&partial) + offset, bufp, buflen);
    return flash_aligned(reinterpret_cast<const Data *>(address), &partial,
                         WORD_ALIGNMENT);
  } else {
    memcpy(reinterpret_cast<uint8_t *>(&partial) + offset, bufp,
           WORD_ALIGNMENT - offset);
    if (!flash_aligned(reinterpret_cast<const Data *>(address), &partial,
                       WORD_ALIGNMENT)) {
      return false;
    }
  }
  address += WORD_ALIGNMENT;
  bufp += WORD_ALIGNMENT - offset;
  buflen -= WORD_ALIGNMENT - offset;

  // Write the aligned buffer middle
  offset = compute_offset(address + buflen, WORD_ALIGNMENT);
  while (buflen > offset) {
    partial = *reinterpret_cast<const Data *>(bufp);
    if (!flash_aligned(reinterpret_cast<const Data *>(address), &partial,
                       WORD_ALIGNMENT)) {
      return false;
    }
    address += WORD_ALIGNMENT;
    bufp += WORD_ALIGNMENT;
    buflen -= WORD_ALIGNMENT;
  }

  // Write the unaligned buffer end
  if (offset > 0) {
    partial = *reinterpret_cast<const Data *>(address);
    memcpy(&partial, bufp, offset);
    if (!flash_aligned(reinterpret_cast<const Data *>(address), &partial,
                       WORD_ALIGNMENT)) {
      return false;
    }
  }
  return  true;
}


bool Flasher_::erase(const uint8_t *address, size_t length) {

  if (length == 0) return true;

  Data partial R2P_FLASH_ALIGNED;

  // Erase the unaligned buffer beginning
  size_t offset = compute_offset(address, WORD_ALIGNMENT);
  address -= offset;
  partial = *reinterpret_cast<const Data *>(address);
  if (offset + length <= WORD_ALIGNMENT) {
    // A single word to write
    memcpy(reinterpret_cast<uint8_t *>(&partial) + offset,
           reinterpret_cast<const uint8_t *>(&erased_word) + offset, length);
    return flash_aligned(reinterpret_cast<const Data *>(address), &partial,
                         WORD_ALIGNMENT);
  } else {
    memcpy(reinterpret_cast<uint8_t *>(&partial) + offset,
           reinterpret_cast<const uint8_t *>(&erased_word) + offset,
           WORD_ALIGNMENT - offset);
    if (!flash_aligned(reinterpret_cast<const Data *>(address), &partial,
                       WORD_ALIGNMENT)) {
      return false;
    }
  }
  address += WORD_ALIGNMENT;
  length -= WORD_ALIGNMENT - offset;

  // Erase the aligned buffer middle
  offset = compute_offset(address + length, WORD_ALIGNMENT);
  while (length > offset) {
    if (!flash_aligned(reinterpret_cast<const Data *>(address), &erased_word,
                       WORD_ALIGNMENT)) {
      return false;
    }
    address += WORD_ALIGNMENT;
    length -= WORD_ALIGNMENT;
  }

  // Erase the unaligned buffer end
  if (offset > 0) {
    partial = *reinterpret_cast<const Data *>(address);
    memcpy(&partial, &erased_word, offset);
    if (!flash_aligned(reinterpret_cast<const Data *>(address), &partial,
                       WORD_ALIGNMENT)) {
      return false;
    }
  }
  return  true;
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

#endif /* R2P_USE_BOOTLOADER */

