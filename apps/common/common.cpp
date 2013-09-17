#include "common.hpp"


unsigned led2pin(unsigned led_id) {
    
    switch (led_id) {
    case 0: return LED1;
    case 1: return LED2;
    case 2: return LED3;
    case 3: return LED4;
    }
    R2P_ASSERT(false && "unreachable code");
    return 0;
}


unsigned pin2led(unsigned pin_id) {
    
    switch (pin_id) {
    case LED1: return 0;
    case LED2: return 1;
    case LED3: return 2;
    case LED4: return 3;
    }
    R2P_ASSERT(false && "unreachable code");
    return 0;
}

