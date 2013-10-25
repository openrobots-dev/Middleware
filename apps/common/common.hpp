#pragma once

#include <r2p/Middleware.hpp>
#include "LedMsg.hpp"

#include <ch.h>
#include <hal.h>


unsigned led2pin(unsigned led_id);
unsigned pin2led(unsigned pin_id);

