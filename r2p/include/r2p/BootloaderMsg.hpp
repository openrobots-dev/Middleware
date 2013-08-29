#pragma once

#include <r2p/common.hpp>
#include <r2p/Message.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/Flasher.hpp>

namespace r2p {


struct IhexRecord {
  enum { MAX_DATA_LENGTH = 16 };

  enum TypeEnum {
    DATA = 0,
    END_OF_FILE,
    EXTENDED_SEGMENT_ADDRESS,
    START_SEGMENT_ADDRESS,
    EXTENDED_LINEAR_ADDRESS,
    START_LINEAR_ADDRESS,
  };

  uint8_t   count;
  uint16_t  offset;
  uint8_t   type;
  uint8_t   data[MAX_DATA_LENGTH];
  uint8_t   checksum;

  uint8_t compute_checksum() const;
} R2P_PACKED;


class BootloaderMsg : public Message {
public:
  enum TypeEnum {
    NACK = 0,
    ACK,
    HEARTBEAT,
    SETUP_REQUEST,
    SETUP_RESPONSE,
    IHEX_RECORD,
    REMOVE_ALL, // TODO
    REMOVE_LAST, // TODO
  };

  enum { MAX_PAYLOAD_LENGTH = 26 };

  typedef Flasher::Address Address;
  typedef Flasher::Length Length;

  struct AckNack {
    uint16_t to_seqn;
  } R2P_PACKED;

  struct SetupRequest {
    Length  pgmlen;
    Length  bsslen;
    Length  datalen;
    Length  stacklen;
    char    name[NamingTraits<Node>::MAX_LENGTH];
    uint8_t checksum; // TODO move outside
  } R2P_PACKED;

  struct SetupResponse {
    Address pgmadr;
    Address bssadr;
    Address dataadr;
    Address datapgmadr;
    uint8_t checksum; // TODO move outside
  } R2P_PACKED;

public:
  uint8_t checksum;
  uint8_t type;
  uint16_t seqn;
  union {
    uint8_t         raw_payload[MAX_PAYLOAD_LENGTH];
    SetupRequest    setup_request;
    SetupResponse   setup_response;
    IhexRecord      ihex_record;
  };

public:
  void cleanup();
  uint8_t compute_checksum() const;
  void update_checksum();
} R2P_PACKED;


inline
void BootloaderMsg::update_checksum() {

  checksum = compute_checksum();
}


} // namespace r2p
