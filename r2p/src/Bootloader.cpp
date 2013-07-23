
#include <r2p/Bootloader.hpp>
#include <r2p/Thread.hpp>

namespace r2p {


bool Bootloader::process_ihex(const IhexRecord &record) {

  switch (record.type) {
  case IhexRecord::DATA: {
    uint32_t start = baseadr + record.offset;
    uint32_t endx = start + record.count;

    if (endx <= tempinfo.pgmadr + tempinfo.pgmlen &&
        start >= tempinfo.pgmadr) {
      // ".text" segment
      return flasher.flash(
        baseadr + record.offset,
        reinterpret_cast<const Flasher::Data *>(record.data),
        record.count
      );
    }
    else if (endx <= tempinfo.datapgmadr + tempinfo.datalen &&
             start >= tempinfo.datapgmadr) {
      // ".data" segment
      return flasher.flash(
        baseadr + record.offset,
        reinterpret_cast<const Flasher::Data *>(record.data),
        record.count
      );
    }
    return false;
  }
  case IhexRecord::END_OF_FILE: {
    if (record.count  != 0) return false;
    if (record.offset != 0) return false;
    if (flasher.end()) {
      state = AWAITING_REQUEST;
      return true;
    } else {
      return false;
    }
  }
  case IhexRecord::EXTENDED_SEGMENT_ADDRESS: {
    if (record.count  != 2) return false;
    if (record.offset != 0) return false;
    baseadr  = (static_cast<uint32_t>(record.data[0]) << 12) |
               (static_cast<uint32_t>(record.data[1]) <<  4);
    return true;
  }
  case IhexRecord::START_SEGMENT_ADDRESS: {
    if (record.count  != 4) return false;
    if (record.offset != 0) return false;
    return true;
  }
  case IhexRecord::EXTENDED_LINEAR_ADDRESS: {
    if (record.count  != 2) return false;
    if (record.offset != 0) return false;
    baseadr  = (static_cast<uint32_t>(record.data[0]) << 24) |
               (static_cast<uint32_t>(record.data[1]) << 16);
    return true;
  }
  case IhexRecord::START_LINEAR_ADDRESS: {
    if (record.count  != 4) return false;
    if (record.offset != 0) return false;
    tempinfo.threadadr  = (static_cast<uint32_t>(record.data[0]) << 24) |
                          (static_cast<uint32_t>(record.data[1]) << 16) |
                          (static_cast<uint32_t>(record.data[2]) <<  8) |
                           static_cast<uint32_t>(record.data[3]);
    return true;
  }
  default: return false;
  }
}


bool Bootloader::compute_addresses() {

  // Update the whole stack size
  if (tempinfo.stacklen == 0) return false;
  tempinfo.stacklen = Thread::compute_stack_size(tempinfo.stacklen);

  // Allocate the program memory
  tempinfo.pgmadr = freeadr = Flasher::align_next(freeadr);
  tempinfo.datapgmadr = tempinfo.pgmadr + Flasher::align_next(tempinfo.pgmlen);
  if (tempinfo.datapgmadr + Flasher::align_next(tempinfo.datalen) >
      Flasher::align_prev(Flasher::get_program_end())) {
    return false;
  }
  if (tempinfo.datalen == 0) {
    tempinfo.datapgmadr = 0;
  }

  // Allocate the ".bss" segment
  tempinfo.bssadr = 0;
  if (tempinfo.bsslen > 0) {
    tempinfo.bssadr = 0; // FIXME chCoreTrim(info.bsslen)
    if (tempinfo.bssadr == 0) return false;
    ::memset(reinterpret_cast<void *>(tempinfo.bssadr), 0x00,
             tempinfo.bsslen);
  }

  // Allocate the ".data" segment
  tempinfo.dataadr = 0;
  if (tempinfo.datalen > 0) {
    tempinfo.dataadr = 0; // FIXME chCoreTrim(info.datalen)
    if (tempinfo.dataadr == 0) return false;
    ::memset(reinterpret_cast<void *>(tempinfo.dataadr), 0xCC,
             tempinfo.datalen);
  }
  return true;
}


bool Bootloader::update_layout(const AppInfo *last_infop) {


  flasher.begin();

  // Number of stored applications
  if (!flasher.flash(
    reinterpret_cast<Flasher::Address>(&flash_layout.numapps),
    reinterpret_cast<const Flasher::Data *>(&numapps),
    sizeof(numapps)
  )) {
    flasher.end();
    return false;
  }

  // Free app program memory address
  if (!flasher.flash(
    reinterpret_cast<Flasher::Address>(&flash_layout.freeadr),
    reinterpret_cast<const Flasher::Data *>(&freeadr),
    sizeof(freeadr)
  )) {
    flasher.end();
    return false;
  }

  // Last application information
  if (numapps > 0 && last_infop != NULL) {
    if (!flasher.flash(
      reinterpret_cast<Flasher::Address>(&flash_layout.infos[numapps - 1]),
      reinterpret_cast<const Flasher::Data *>(last_infop),
      sizeof(AppInfo)
    )) {
      flasher.end();
      return false;
    }
  }

  return flasher.end();
}


bool Bootloader::process(const BootloaderMsg &request_msg,
                         BootloaderMsg &response_msg) {

  switch (request_msg.type) {
  case BootloaderMsg::NACK:
  case BootloaderMsg::ACK:
  case BootloaderMsg::SETUP_RESPONSE: {
    return false; // Do not reply, could have been generated by self
  }
  case BootloaderMsg::SETUP_REQUEST: {
    if (state == RECEIVING_IHEX) {
      flasher.end();
      state = AWAITING_REQUEST;
      return false;
    } else if (state != AWAITING_REQUEST) {
      state = AWAITING_REQUEST;
      return false;
    }

    tempinfo.pgmlen = request_msg.setup_request.pgmlen;
    tempinfo.bsslen = request_msg.setup_request.bsslen;
    tempinfo.datalen = request_msg.setup_request.datalen;
    tempinfo.stacklen = request_msg.setup_request.stacklen;

    if (compute_addresses()) {
      response_msg.setup_response.pgmadr = tempinfo.pgmadr;
      response_msg.setup_response.bssadr = tempinfo.bssadr;
      response_msg.setup_response.dataadr = tempinfo.dataadr;
      response_msg.setup_response.datapgmadr = tempinfo.datapgmadr;
      response_msg.type = BootloaderMsg::SETUP_RESPONSE;

      state = RECEIVING_IHEX;
      flasher.begin();
    } else {
      response_msg.type = BootloaderMsg::NACK;
    }
    return true;
  }
  case BootloaderMsg::IHEX_RECORD: {
    if (state == RECEIVING_IHEX && process_ihex(request_msg.ihex_record)) {
      response_msg.type = BootloaderMsg::ACK;
    } else {
      response_msg.type = BootloaderMsg::NACK;
      state = AWAITING_REQUEST;
    }
    return true;
  }
  default: {
    if (state == RECEIVING_IHEX) {
      flasher.end();
    }
    response_msg.type = BootloaderMsg::NACK;
    state = AWAITING_REQUEST;
    return true;
  }
  }
}


Bootloader::Bootloader(Flasher::Data *flash_page_bufp)
:
  state(AWAITING_REQUEST),
  flasher(flash_page_bufp),
  numapps(flash_layout.numapps),
  baseadr(0),
  freeadr(flash_layout.freeadr)
{}


} // namespace r2p
