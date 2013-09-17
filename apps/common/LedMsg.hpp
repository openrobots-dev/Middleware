#pragma once

#include <r2p/Message.hpp>


struct LedMsg : public r2p::Message {
    unsigned    id;
    bool        on;
} R2P_PACKED;

