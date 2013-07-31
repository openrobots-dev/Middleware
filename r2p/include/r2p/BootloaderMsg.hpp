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

  uint8_t   type;
  uint8_t   checksum;
  uint16_t  offset;
  uint8_t   count;
  uint8_t   data[MAX_DATA_LENGTH];
} R2P_PACKED;


class BootloaderMsg : public Message {
public:
  enum TypeEnum {
    NACK = 0,
    ACK,
    SETUP_REQUEST,
    SETUP_RESPONSE,
    IHEX_RECORD,
  };

  typedef Flasher::Address Address;
  typedef Flasher::Length Length;

  struct SetupRequest {
    Length  pgmlen;
    Length  bsslen;
    Length  datalen;
    Length  stacklen;
    char    name[NamingTraits<Node>::MAX_LENGTH];
    uint8_t namelen;
    uint8_t checksum;
  } R2P_PACKED;

  struct SetupResponse {
    Address pgmadr;
    Address bssadr;
    Address dataadr;
    Address datapgmadr;
  } R2P_PACKED;

public:
  union {
    SetupRequest    setup_request;
    SetupResponse   setup_response;
    IhexRecord      ihex_record;
  };
  uint8_t type;
} R2P_PACKED;


} // namespace r2p
