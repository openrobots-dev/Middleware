
#include <r2p/Bootloader.hpp>
#include <r2p/Thread.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

namespace r2p {


const uint32_t Bootloader::an_invalid_signature R2P_FLASH_ALIGNED =
  static_cast<uint32_t>(~SIGNATURE);


bool Bootloader::remove_last() {

  Iterator i = begin();
  if (!i->is_valid()) return false;
  for (; i->is_valid() && i->get_next()->is_valid(); ++i) {}

  unreserve_ram(i->bsslen);
  unreserve_ram(i->datalen);

  begin_write();
  write(&*i, &an_invalid_signature, sizeof(an_invalid_signature));
  end_write();
  return true;
}


bool Bootloader::remove_all() {

  const FlashAppInfo *headp = &*begin();
  if (!headp->is_valid()) return false;

  begin_write();
  write(headp, &an_invalid_signature, sizeof(an_invalid_signature));
  end_write();
  return true;
}


void Bootloader::spin_loop() {

  for (;;) {
    fetch();
    switch (master_msgp->type) {
    case BootMsg::BEGIN_LOADER: {
      release();
      begin_write();
      do_loader();
      end_write();
      break;
    }
    case BootMsg::BEGIN_APPINFO: {
      release();
      do_appinfo();
      break;
    }
    case BootMsg::BEGIN_GETPARAM: {
      release();
      do_getparam();
      break;
    }
    case BootMsg::BEGIN_SETPARAM: {
      release();
      begin_write();
      do_setparam();
      end_write();
      break;
    }
    default: {
      do_release_error();
      break;
    }
    }
  }
}


void Bootloader::alloc() {

  R2P_ASSERT(pubp != NULL);
  R2P_ASSERT(slave_msgp == NULL);

  while (!pubp->alloc(slave_msgp)) {
    Thread::sleep(Time::ms(100)); // TODO: configure
  }
  Message::clean(*slave_msgp);
}


void Bootloader::publish() {

  R2P_ASSERT(pubp != NULL);
  R2P_ASSERT(slave_msgp != NULL);

  while (!pubp->publish_remotely(*slave_msgp)) {
    Thread::sleep(Time::ms(100)); // TODO: Configure
  }
  slave_msgp = NULL;
}


void Bootloader::fetch() {

  R2P_ASSERT(subp != NULL);
  R2P_ASSERT(master_msgp == NULL);

  while (!subp->fetch(master_msgp)) {
    Thread::sleep(Time::ms(100)); // TODO: configure
  }
}


void Bootloader::release() {

  R2P_ASSERT(subp != NULL);
  R2P_ASSERT(master_msgp != NULL);

  subp->release(*master_msgp);
  master_msgp = NULL;
}


void Bootloader::do_loader() {

  do_ack();

  for (;;) {
    // Check if receiving a new app, or exiting the procedure
    fetch();
    switch (master_msgp->type) {
    case BootMsg::LINKING_SETUP: break;
    case BootMsg::END_LOADER: {
      do_ack();
      return;
    }
    default: {
      do_release_error();
      return;
    }
    }

    // Receive the setup parameters (segment lengths, app name, etc.)
    BootMsg::LinkingSetup setup = master_msgp->linking_setup;
    release();

    // Compute the linker addresses
    BootMsg::LinkingAddresses addresses;
    if (!compute_addresses(setup, addresses)) {
      do_error();
      return;
    }
    alloc(BootMsg::LINKING_ADDRESSES);
    slave_msgp->linking_addresses = addresses;
    publish();

    // Receive the linking outcome
    if (!fetch(BootMsg::LINKING_OUTCOME)) {
      do_release_error();
      return;
    }
    BootMsg::LinkingOutcome outcome = master_msgp->linking_outcome;
    release();
    do_ack();

    // Receive and process IHEX records
    for (;;) {
      if (!fetch(BootMsg::IHEX_RECORD)) {
        do_release_error();
        return;
      }
      IhexRecord &record = master_msgp->ihex_record;
      if (!process_ihex(record, setup, addresses)) {
        do_release_error();
        return;
      }
      if (record.type == IhexRecord::END_OF_FILE) {
        break;
      }
      release();
      do_ack();
    }
    release();
    do_ack();

    // Flash the new application information
    if (!write_appinfo(
          reinterpret_cast<const FlashAppInfo *>(addresses.infoadr),
          setup, addresses, outcome)) {
      do_error();
      return;
    }

    do_ack();
  }
}


void Bootloader::do_appinfo() {

  alloc(BootMsg::APPINFO_SUMMARY);
  {
    BootMsg::AppInfoSummary &summary = slave_msgp->appinfo_summary;
    summary.numapps = get_num_apps();
    summary.freeadr = get_free_address();
  }
  publish();

  if (!fetch(BootMsg::ACK)) {
    do_release_error();
    return;
  }

  for (Iterator i = begin(); i->is_valid(); ++i) {
    alloc(BootMsg::LINKING_SETUP);
    {
      BootMsg::LinkingSetup &setup = slave_msgp->linking_setup;
      setup.pgmlen = i->pgmlen;
      setup.bsslen = i->bsslen;
      setup.datalen = i->datalen;
      setup.stacklen = i->stacklen;
      strncpy(setup.name, i->name, NamingTraits<Node>::MAX_LENGTH);
    }
    publish();

    if (!fetch(BootMsg::ACK)) {
      do_release_error();
      return;
    }

    alloc(BootMsg::LINKING_ADDRESSES);
    {
      BootMsg::LinkingAddresses &addresses = slave_msgp->linking_addresses;
      addresses.infoadr = reinterpret_cast<const uint8_t *>(&*i);
      addresses.pgmadr = i->get_program();
      addresses.bssadr = i->bssp;
      addresses.dataadr = i->datap;
      addresses.datapgmadr = i->get_data_program();
      addresses.freeadr = get_free_address();
    }
    publish();

    if (!fetch(BootMsg::ACK)) {
      do_release_error();
      return;
    }

    alloc(BootMsg::LINKING_OUTCOME);
    {
      BootMsg::LinkingOutcome &outcome = slave_msgp->linking_outcome;
      outcome.threadadr = reinterpret_cast<BootMsg::Address>(i->threadf);
      outcome.cfgadr = i->cfgp;
      outcome.appinfoadr = reinterpret_cast<BootMsg::Address>(&*i);
    }
    publish();

    if (!fetch(BootMsg::ACK)) {
      do_release_error();
      return;
    }
  }

  alloc(BootMsg::END_APPINFO);
  publish();
  if (!fetch(BootMsg::ACK)) {
    do_release_error();
    return;
  }
  do_ack();
}


void Bootloader::do_getparam() {

  enum { MAX_LENGTH = BootMsg::ParamChunk::MAX_DATA_LENGTH };

  do_ack();

  for (;;) {
    // Check if sending a parameter or exiting the procedure
    fetch();
    switch (master_msgp->type) {
    case BootMsg::PARAM_REQUEST: break;
    case BootMsg::END_GETPARAM: {
      release();
      do_ack();
      return;
    }
    default: {
      do_release_error();
      return;
    }
    }

    // Receive the parameter identifier
    BootMsg::ParamRequest &request = master_msgp->param_request;
    const FlashAppInfo *infop = get_app(request.appname);
    R2P_ASSERT(infop != NULL);
    const uint8_t *sourcep = infop->cfgp;
    if (!Flasher::is_program(sourcep)) {
      do_release_error();
      return;
    }
    sourcep += request.offset;
    size_t length = request.length;
    release();

    // Send all the needed chunks
    while (length > 0) {
      alloc(BootMsg::PARAM_CHUNK);
      BootMsg::ParamChunk &chunk = slave_msgp->param_chunk;
      if (length >= MAX_LENGTH) {
        memcpy(chunk.data, sourcep, MAX_LENGTH);
        length -= MAX_LENGTH;
      } else {
        memcpy(chunk.data, sourcep, length);
        length = 0;
      }
      publish();
      if (!fetch(BootMsg::ACK)) {
        do_release_error();
        return;
      }
    }
  }
}


void Bootloader::do_setparam() {

  enum { MAX_LENGTH = BootMsg::ParamChunk::MAX_DATA_LENGTH };

  do_ack();

  for (;;) {
    // Check if receiving a parameter or exiting the procedure
    fetch();
    switch (master_msgp->type) {
    case BootMsg::PARAM_REQUEST: break;
    case BootMsg::END_SETPARAM: {
      do_ack();
      return;
    }
    default: {
      do_release_error();
      return;
    }
    }

    // Receive the parameter identifier
    BootMsg::ParamRequest &request = master_msgp->param_request;
    const FlashAppInfo *infop = get_app(request.appname);
    R2P_ASSERT(infop != NULL);
    const uint8_t *sourcep = infop->cfgp;
    if (!Flasher::is_program(sourcep)) {
      do_release_error();
      return;
    }
    sourcep += request.offset;
    size_t length = request.length;
    release();

    // Receive all the needed chunks
    while (length > 0) {
      if (!fetch(BootMsg::PARAM_CHUNK)) {
        do_release_error();
        return;
      }
      BootMsg::ParamChunk &chunk = master_msgp->param_chunk;
      if (length >= MAX_LENGTH) {
        if (!write(sourcep, chunk.data, MAX_LENGTH)) {
          do_release_error();
          return;
        }
        length -= MAX_LENGTH;
      } else {
        if (!write(sourcep, chunk.data, length)) {
          do_release_error();
          return;
        }
        length = 0;
      }
      release();
      do_ack();
    }
  }
}


void Bootloader::do_ack() {

  alloc(BootMsg::ACK);
  publish();
}


void Bootloader::do_error() {

  alloc(BootMsg::NACK);
  publish();
}


void Bootloader::do_release_error() {

  release();
  alloc(BootMsg::NACK);
  publish();
}


bool Bootloader::write_appinfo(const FlashAppInfo *flashinfop,
                               const BootMsg::LinkingSetup &setup,
                               const BootMsg::LinkingAddresses &addresses,
                               const BootMsg::LinkingOutcome &outcome) {

#define WRITE_(dstfield, srcvalue) \
  if (!write(reinterpret_cast<const void *>(&flashinfop->dstfield), \
             reinterpret_cast<const void *>(&srcvalue), \
             sizeof(flashinfop->dstfield))) return false;

  uint32_t signature = SIGNATURE;
  WRITE_(signature, signature);
  WRITE_(pgmlen, setup.pgmlen);
  WRITE_(bsslen, setup.bsslen);
  WRITE_(datalen, setup.datalen);
  WRITE_(bssp, addresses.bssadr);
  WRITE_(datap, addresses.dataadr);
  WRITE_(cfgp, outcome.cfgadr);
  WRITE_(stacklen, setup.stacklen);
  WRITE_(threadf, outcome.threadadr);

  if (!write(flashinfop->name, setup.name, NamingTraits<Node>::MAX_LENGTH)) {
    return false;
  }

  return true;

#undef WRITE_
}


bool Bootloader::process_ihex(const IhexRecord &record,
                              const BootMsg::LinkingSetup &setup,
                              const BootMsg::LinkingAddresses &addresses) {

  // Check the checksum
  if (record.checksum != record.compute_checksum()) return false;

  // Select the record type
  switch (record.type) {
  case IhexRecord::DATA: {
    const uint8_t *startp = basep + record.offset;
    const uint8_t *endxp = startp + record.count;

    if (endxp  <= addresses.pgmadr + setup.pgmlen &&
        startp >= addresses.pgmadr) {
      // ".text" segment
      return write(basep + record.offset, record.data, record.count);
    }
    else if (endxp  <= addresses.datapgmadr + setup.datalen &&
             startp >= addresses.datapgmadr) {
      // ".data" segment
      return write(basep + record.offset, record.data, record.count);
    }
    return false;
  }
  case IhexRecord::END_OF_FILE: {
    if (record.count  != 0) return false;
    if (record.offset != 0) return false;
    return true;
  }
  case IhexRecord::EXTENDED_SEGMENT_ADDRESS: {
    if (record.count  != 2) return false;
    if (record.offset != 0) return false;
    basep = reinterpret_cast<const uint8_t *>(
      (static_cast<uintptr_t>(record.data[0]) << 12) |
      (static_cast<uintptr_t>(record.data[1]) <<  4)
    );
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
    basep = reinterpret_cast<const uint8_t *>(
      (static_cast<uintptr_t>(record.data[0]) << 24) |
      (static_cast<uintptr_t>(record.data[1]) << 16)
    );
    return true;
  }
  case IhexRecord::START_LINEAR_ADDRESS: {
    if (record.count  != 4) return false;
    if (record.offset != 0) return false;
    return true;
  }
  default: return false;
  }
}


Bootloader::Bootloader(Flasher::Data *flash_page_bufp,
                       Publisher<BootMsg> &pub,
                       Subscriber<BootMsg> &sub)
:
  basep(NULL),
  flasher(flash_page_bufp),
  pubp(&pub),
  subp(&sub),
  master_msgp(NULL),
  slave_msgp(NULL)
{
  R2P_ASSERT(flash_page_bufp != NULL);
}


const Bootloader::Iterator Bootloader::end() {

  Iterator i = begin();
  while (i->is_valid()) {
    ++i;
  }
  return i;
}


size_t Bootloader::get_num_apps() {

  size_t total = 0;
  for (Iterator i = begin(); i->is_valid(); ++i) {
    ++total;
  }
  return total;
}


const uint8_t *Bootloader::get_free_address() {

  return reinterpret_cast<const uint8_t *>(&*end());
}


const Bootloader::FlashAppInfo *Bootloader::get_app(const char *appname) {

  for (Iterator i = begin(); i->is_valid(); ++i) {
    if (i->has_name(appname)) {
      return &*i;
    }
  }
  return NULL;
}


bool Bootloader::compute_addresses(const BootMsg::LinkingSetup &setup,
                                   BootMsg::LinkingAddresses &addresses) {

  memset(&addresses, 0, sizeof(addresses));

  // Check the stack size
  if (setup.stacklen == 0) return false;

  // Allocate the program memory parts:
  // (previous/OS) | app_info | .text | .data[const] | (free)
  const FlashAppInfo *newp = &*end();
  addresses.infoadr = reinterpret_cast<const uint8_t *>(newp);
  addresses.pgmadr = newp->get_program();
  addresses.datapgmadr = newp->get_data_program();
  addresses.freeadr = reinterpret_cast<const uint8_t *>(newp->get_next());
  if (addresses.infoadr >= Flasher::align_prev(Flasher::get_program_end()) ||
      addresses.freeadr >  Flasher::align_prev(Flasher::get_program_end())) {
    return false;
  }

  // Allocate the ".bss" segment
  if (setup.bsslen > 0) {
    uint8_t *bssp = reserve_ram(setup.bsslen);
    addresses.bssadr = bssp;
    if (bssp == NULL) return false;
    memset(bssp, 0x00, setup.bsslen);
  }

  // Allocate the ".data" segment
  if (setup.datalen > 0) {
    uint8_t *datap = reserve_ram(setup.datalen);
    addresses.dataadr = datap;
    if (datap == NULL) {
      unreserve_ram(setup.bsslen);
      addresses.bssadr = NULL;
      return false;
    }
    memset(datap, 0xCC, setup.datalen);
  }
  return true;
}


} // namespace r2p
