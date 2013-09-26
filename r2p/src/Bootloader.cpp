
#include <r2p/Bootloader.hpp>
#include <r2p/Thread.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>

#if R2P_USE_BOOTLOADER
namespace r2p {


const uint32_t Bootloader::an_invalid_signature R2P_FLASH_ALIGNED =
  static_cast<uint32_t>(~SIGNATURE);


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
      do_release_error(__LINE__);
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
  Message::reset_payload(*slave_msgp);
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
      release();
      do_ack();
      return;
    }
    case BootMsg::REMOVE_LAST: {
      release();
      if (!remove_last()) {
        do_error(__LINE__);
        return;
      }
      do_ack();
      continue;
    }
    case BootMsg::REMOVE_ALL: {
      release();
      if (!remove_all()) {
        do_error(__LINE__);
        return;
      }
      do_ack();
      continue;
    }
    default: {
      do_release_error(__LINE__);
      return;
    }
    }

    // Receive the setup parameters (segment lengths, app name, etc.)
    BootMsg::LinkingSetup setup = master_msgp->linking_setup;
    release();

    // Check if the app already exists
    if (get_app(setup.name) != NULL) {
      do_error(__LINE__);
      return;
    }

    // Compute the linker addresses
    BootMsg::LinkingAddresses addresses;
    if (!compute_addresses(setup, addresses)) {
      do_error(__LINE__);
      return;
    }
    alloc(BootMsg::LINKING_ADDRESSES);
    slave_msgp->linking_addresses = addresses;
    publish();

    // Receive the linking outcome
    if (!fetch(BootMsg::LINKING_OUTCOME)) {
      do_release_error(__LINE__);
      return;
    }
    BootMsg::LinkingOutcome outcome = master_msgp->linking_outcome;
    release();
    do_ack();

    // Receive and process IHEX records
    for (;;) {
      if (!fetch(BootMsg::IHEX_RECORD)) {
        do_release_error(__LINE__);
        return;
      }
      IhexRecord &record = master_msgp->ihex_record;
      if (!process_ihex(record, setup, addresses)) {
        do_release_error(__LINE__);
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
      do_error(__LINE__);
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
    summary.pgmstartadr = Flasher::get_program_start();
    summary.pgmendadr = Flasher::get_program_end();
    summary.ramstartadr = Flasher::get_ram_start();
    summary.ramendadr = Flasher::get_ram_end();
  }
  publish();

  if (!fetch(BootMsg::ACK)) {
    do_release_error(__LINE__);
    return;
  }
  release();

  for (Iterator i = begin(); i != end(); ++i) {
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
      do_release_error(__LINE__);
      return;
    }
    release();

    alloc(BootMsg::LINKING_ADDRESSES);
    {
      BootMsg::LinkingAddresses &addresses = slave_msgp->linking_addresses;
      addresses.infoadr = reinterpret_cast<const uint8_t *>(&*i);
      addresses.pgmadr = i->get_program();
      addresses.bssadr = i->bssp;
      addresses.dataadr = i->datap;
      addresses.datapgmadr = i->get_data_program();
      addresses.nextadr = reinterpret_cast<const uint8_t *>(i->get_next());
    }
    publish();

    if (!fetch(BootMsg::ACK)) {
      do_release_error(__LINE__);
      return;
    }
    release();

    alloc(BootMsg::LINKING_OUTCOME);
    {
      BootMsg::LinkingOutcome &outcome = slave_msgp->linking_outcome;
      outcome.mainadr = reinterpret_cast<BootMsg::Address>(i->mainf);
      outcome.cfgadr = i->cfgp;
      outcome.cfglen = i->cfglen;
      outcome.ctorsadr = i->ctorsp;
      outcome.ctorslen = i->ctorslen;
      outcome.dtorsadr = i->dtorsp;
      outcome.dtorslen = i->dtorslen;
    }
    publish();

    if (!fetch(BootMsg::ACK)) {
      do_release_error(__LINE__);
      return;
    }
    release();
  }

  alloc(BootMsg::END_APPINFO);
  publish();
  if (!fetch(BootMsg::ACK)) {
    do_release_error(__LINE__);
    return;
  }
  release();
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
      do_release_error(__LINE__);
      return;
    }
    }

    // Receive the parameter identifier
    BootMsg::ParamRequest &request = master_msgp->param_request;
    if (request.length == 0) {
      do_release_error(__LINE__);
      return;
    }
    const FlashAppInfo *infop = get_app(request.appname);
    if (infop == NULL) {
      do_release_error(__LINE__);
      return;
    }
    const uint8_t *sourcep = (request.offset != ~0u)
                             ? &infop->cfgp[request.offset]
                             : reinterpret_cast<const uint8_t *>(&infop->flags);
    if (!check_bounds(sourcep, request.length, infop->cfgp, infop->cfglen) &&
        !check_bounds(sourcep, request.length, &infop->flags,
                      sizeof(FlashAppInfo::Flags))) {
      do_release_error(__LINE__);
      return;
    }
    size_t length = request.length;
    release();

    // Send all the needed chunks
    while (length > 0) {
      alloc(BootMsg::PARAM_CHUNK);
      BootMsg::ParamChunk &chunk = slave_msgp->param_chunk;
      if (length >= MAX_LENGTH) {
        memcpy(chunk.data, sourcep, MAX_LENGTH);
        sourcep += MAX_LENGTH;
        length -= MAX_LENGTH;
      } else {
        memcpy(chunk.data, sourcep, length);
        length = 0;
      }
      publish();
      if (!fetch(BootMsg::ACK)) {
        do_release_error(__LINE__);
        return;
      }
      release();
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
      release();
      do_ack();
      return;
    }
    default: {
      do_release_error(__LINE__);
      return;
    }
    }

    // Receive the parameter identifier
    BootMsg::ParamRequest &request = master_msgp->param_request;
    if (request.length == 0) {
      do_release_error(__LINE__);
      return;
    }
    const FlashAppInfo *infop = get_app(request.appname);
    if (infop == NULL) {
      do_release_error(__LINE__);
      return;
    }
    const uint8_t *targetp = (request.offset != ~0u)
                             ? &infop->cfgp[request.offset]
                             : reinterpret_cast<const uint8_t *>(&infop->flags);
    if (!check_bounds(targetp, request.length, infop->cfgp, infop->cfglen) &&
        !check_bounds(targetp, request.length, &infop->flags,
                      sizeof(FlashAppInfo::Flags))) {
      do_release_error(__LINE__);
      return;
    }
    size_t length = request.length;
    release();
    do_ack();

    // Receive all the needed chunks
    while (length > 0) {
      if (!fetch(BootMsg::PARAM_CHUNK)) {
        do_release_error(__LINE__);
        return;
      }
      BootMsg::ParamChunk &chunk = master_msgp->param_chunk;
      if (length >= MAX_LENGTH) {
        if (!write(targetp, chunk.data, MAX_LENGTH)) {
          do_release_error(__LINE__);
          return;
        }
        targetp += MAX_LENGTH;
        length -= MAX_LENGTH;
      } else {
        if (!write(targetp, chunk.data, length)) {
          do_release_error(__LINE__);
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


void Bootloader::do_error(uint32_t line, const char *textp) {

  alloc(BootMsg::NACK);
  slave_msgp->error_info.line = line;
  strncpy(slave_msgp->error_info.text, textp,
          BootMsg::ErrorInfo::MAX_TEXT_LENGTH);
  publish();
}


void Bootloader::do_release_error(uint32_t line, const char *textp) {

  release();
  alloc(BootMsg::NACK);
  slave_msgp->error_info.line = line;
  strncpy(slave_msgp->error_info.text, textp,
          BootMsg::ErrorInfo::MAX_TEXT_LENGTH);
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
  WRITE_(cfglen, outcome.cfglen);
  WRITE_(stacklen, setup.stacklen);
  WRITE_(mainf, outcome.mainadr);
  WRITE_(ctorsp, outcome.ctorsadr);
  WRITE_(ctorslen, outcome.ctorslen);
  WRITE_(dtorsp, outcome.dtorsadr);
  WRITE_(dtorslen, outcome.dtorslen);
  if (!write(flashinfop->name, setup.name, NamingTraits<Node>::MAX_LENGTH)) {
    return false;
  }
  WRITE_(flags.raw, setup.flags);
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
             startp >= addresses.datapgmadr && setup.datalen > 0) {
      // ".data" segment
      R2P_ASSERT(addresses.datapgmadr != NULL);
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


bool Bootloader::remove_last() {

  const FlashAppInfo *const lastp = get_last_app();
  if (lastp == NULL) return false;

  unreserve_ram(lastp->bsslen);
  unreserve_ram(lastp->datalen);

  write(lastp, &an_invalid_signature, sizeof(an_invalid_signature));
  return true;
}


bool Bootloader::remove_all() {

  const FlashAppInfo *lastp = get_last_app();
  while (lastp != NULL) {
    unreserve_ram(lastp->bsslen);
    unreserve_ram(lastp->datalen);

    Iterator i = begin();
    if (lastp == i) break;
    for (; i->get_next() != lastp; ++i) {}
    lastp = i;
  }

  bool success; (void)success;
  success = flasher.erase(Flasher::get_program_start(),
                          Flasher::get_program_length());
  R2P_ASSERT(success);
  return true;
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


size_t Bootloader::get_num_apps() {

  size_t total = 0;
  for (Iterator i = begin(); i->is_valid(); ++i) {
    ++total;
  }
  return total;
}


const Bootloader::FlashAppInfo *Bootloader::get_free_app() {

  Iterator i = begin();
  while (i != end()) ++i;
  return (Flasher::is_within_bounds(&*i)) ? i : NULL;
}


const Bootloader::FlashAppInfo *Bootloader::get_last_app() {

  const FlashAppInfo *prevp = NULL;
  for (Iterator i = begin(); i != end(); ++i) {
    prevp = i;
  }
  return Flasher::is_within_bounds(prevp) ? prevp : NULL;
}


const Bootloader::FlashAppInfo *Bootloader::get_app(const char *appname) {

  for (Iterator i = begin(); i != end(); ++i) {
    if (i->has_name(appname)) {
      return i;
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
  const FlashAppInfo *newp = get_free_app();
  if (newp == NULL) return false;
  addresses.infoadr = reinterpret_cast<const uint8_t *>(newp);
  addresses.pgmadr = FlashAppInfo::compute_program(newp);
  addresses.datapgmadr =
    FlashAppInfo::compute_data_program(newp, setup.pgmlen, setup.datalen);
  addresses.nextadr = reinterpret_cast<const uint8_t *>(
    FlashAppInfo::compute_next(newp, setup.pgmlen, setup.datalen)
  );
  if (addresses.infoadr >= Flasher::get_program_end() ||
      addresses.nextadr >  Flasher::get_program_end()) {
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


bool Bootloader::launch(const char *appname) {

  const FlashAppInfo *const infop = get_app(appname);
  if (infop == NULL) return false;
  if (!infop->flags.enabled) return true;

  char *namep = new char[NamingTraits<Node>::MAX_LENGTH + 1];
  if (namep == NULL) return false;
  strncpy(namep, infop->name, NamingTraits<Node>::MAX_LENGTH);
  namep[NamingTraits<Node>::MAX_LENGTH] = 0;

  Thread *threadp = Thread::create_heap(
    NULL, infop->stacklen, Thread::NORMAL,
    app_threadf_wrapper, const_cast<FlashAppInfo *>(infop), namep
  );
  return threadp != NULL;
}


Thread::Return Bootloader::app_threadf_wrapper(Thread::Argument appinfop) {

  typedef FlashAppInfo::Proc Proc;

  register const Bootloader::FlashAppInfo *infop =
    reinterpret_cast<const Bootloader::FlashAppInfo *>(appinfop);

  // Clear the ".bss" section
  memset(const_cast<uint8_t *>(infop->bssp), 0, infop->bsslen);

  // Copy the ".data" section
  memcpy(const_cast<uint8_t *>(infop->datap), infop->get_data_program(),
         infop->datalen);

  // Call global constructors
  for (size_t i = 0; i < infop->ctorslen; ++i) {
    (*reinterpret_cast<const Proc *>(&infop->ctorsp[i * sizeof(Proc)]))();
  }

  // Call the app entry point procedure
  infop->mainf();

  // Call global destructors
  for (size_t i = 0; i < infop->dtorslen; ++i) {
    (*reinterpret_cast<const Proc *>(&infop->dtorsp[i * sizeof(Proc)]))();
  }

  return Thread::OK;
}


bool Bootloader::launch_all() {

  bool all_launched = true;
  for (Iterator i = begin(); i != end(); ++i) {
    if (!launch(i->name)) {
      all_launched = false;
    }
  }
  return all_launched;
}


} // namespace r2p
#endif // R2P_USE_BOOTLOADER
